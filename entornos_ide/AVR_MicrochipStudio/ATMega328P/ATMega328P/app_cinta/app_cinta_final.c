#include "app_cinta_final.h"

// ============================================================================
// PROTOTIPOS DE FUNCIONES PRIVADAS (Uso interno)
// ============================================================================
static uint8_t Calcular_Destino(uint8_t altura);
static bool Cola_Vacia(sColaCajasFinal* cola);
static void Enqueue_Caja(sColaCajasFinal* cola, uint8_t altura_medida);
static sCajaFinal* Peek_Caja(sColaCajasFinal* cola);
static sCajaFinal Dequeue_Caja(sColaCajasFinal* cola);

// ============================================================================
// VARIABLES ESTÁTICAS (Memoria privada del módulo)
// ============================================================================

// 1. Estado de la máquina principal
static eCintaFinalState estado_medicion = CINTA_FINAL_IDLE;

// 2. Control de tiempos (Heartbeat y Cinemática)
static uint32_t timeout_hb = 0;
static uint32_t tick_hb = 0;
static uint32_t t_inicio_caja = 0;
static uint32_t ultimo_delta_t_s0 = 1000; // Valor seguro inicial de velocidad

// 3. Arreglos físicos (Servos, Sensores y Filtros)
static uint8_t config_alturas_salida[3] = {ALTURA_CAJA_CHICA, ALTURA_CAJA_MEDIANA, ALTURA_CAJA_GRANDE};
static sServoFinal control_servos[3];
static Debouncer_t filtro_sensores[4];

// Los 4 sensores arrancan en estado 1 lógico (por las resistencias Pull-Up)
static sSensorFinal control_sensores[4] = {
    {1, 1}, {1, 1}, {1, 1}, {1, 1}
};

// 4. Memoria de Tránsito (Colas FIFO de las cajas)
static sColaCajasFinal colas_zonas[3];

// 5. Puntero a la función de error (Callback)
static CintaFinalErrorCallback_t cb_error = NULL;

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
void App_CintaFinal_Init(void) {
    estado_medicion = CINTA_FINAL_IDLE;
    
    // Inicializamos el driver asíncrono del ultrasónico y el hardware PWM
    HCSR04_Init();     
    HAL_Servo_Init();

    // Inicializamos los 4 filtros anti-rebote lógicos
    // Parámetros: puntero al filtro, tiempo de validación (ej. 20ms), estado inicial (true = HIGH)
    for(uint8_t i = 0; i < 4; i++) {
        Debounce_Init(&filtro_sensores[i], 20, true); 
    }
}

// ============================================================================
// API DE CONTROL PÚBLICA (Llamadas desde comandos.c)
// ============================================================================

void App_CintaFinal_SetEstado(bool encender) {
    if (encender) {
        estado_medicion = CINTA_FINAL_IDLE; 
    } else {
        estado_medicion = CINTA_FINAL_OFF; 
    }
    // Despachamos la orden física a la capa BSP (Hardware)
    GPIO_SetCinta(encender); 
}

void App_CintaFinal_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada) {
    if (salida_idx < 3) {
        config_alturas_salida[salida_idx] = altura_asignada;
    }
}

void App_CintaFinal_SetErrorCallback(CintaFinalErrorCallback_t callback) {
    cb_error = callback;
}

// ============================================================================
// FUNCIONES PRIVADAS: MATEMÁTICA Y LÓGICA DE NEGOCIO
// ============================================================================

static uint8_t Calcular_Destino(uint8_t altura) {
    // Comparamos usando la matriz dinámica de tolerancias paramétricas
    if (altura >= (config_alturas_salida[0] - TOLERANCIA_MEDICION_CM) && 
        altura <= (config_alturas_salida[0] + TOLERANCIA_MEDICION_CM)) {
        return 1; // Sale por Servo 1
    } 
    else if (altura >= (config_alturas_salida[1] - TOLERANCIA_MEDICION_CM) && 
             altura <= (config_alturas_salida[1] + TOLERANCIA_MEDICION_CM)) {
        return 2; // Sale por Servo 2
    } 
    else if (altura >= (config_alturas_salida[2] - TOLERANCIA_MEDICION_CM) && 
             altura <= (config_alturas_salida[2] + TOLERANCIA_MEDICION_CM)) {
        return 3; // Sale por Servo 3
    }
    return 0; // Descarte o tamaño no reconocido
}

// ============================================================================
// FUNCIONES PRIVADAS: MANEJO DE MEMORIA (Ring Buffers)
// ============================================================================

static bool Cola_Vacia(sColaCajasFinal* cola) {
    return (cola->count == 0);
}

static void Enqueue_Caja(sColaCajasFinal* cola, uint8_t altura_medida) {
    // Seguridad: Solo escribimos si el buffer no está saturado
    if (cola->count < MAX_CAJAS_EN_CINTA) {
        // 1. Cargamos los datos en la posición libre (head)
        cola->buffer[cola->head].altura = altura_medida;
        cola->buffer[cola->head].destino_salida = Calcular_Destino(altura_medida);
        
        // 2. Avanzamos el puntero de forma circular matemática
        cola->head = (cola->head + 1) % MAX_CAJAS_EN_CINTA;
        cola->count++;
    }
}

static sCajaFinal* Peek_Caja(sColaCajasFinal* cola) {
    if (cola->count > 0) {
        // Devolvemos el puntero a la caja más antigua (sin borrarla)
        return &(cola->buffer[cola->tail]);
    }
    return NULL;
}

static sCajaFinal Dequeue_Caja(sColaCajasFinal* cola) {
    sCajaFinal caja_salida = {0, 0, 0}; 
    if (cola->count > 0) {
        // 1. Extraemos una copia de la caja más antigua
        caja_salida = cola->buffer[cola->tail];
        
        // 2. Liberamos ese espacio avanzando el puntero de cola
        cola->tail = (cola->tail + 1) % MAX_CAJAS_EN_CINTA;
        cola->count--;
    }
    return caja_salida;
}

// ============================================================================
// MÁQUINA DE ESTADOS PRINCIPAL (VERSIÓN DE PRUEBA 1: HEARTBEAT Y COMANDO)
// ============================================================================

void App_CintaFinal_Task(void) {
    // 1. Lectura del hardware base (Sensor S0) y filtrado anti-rebote
    bool raw_s0 = GPIO_LeerSensor(0);
    control_sensores[0].actual_state = Debounce_Update(&filtro_sensores[0], raw_s0);

    // 2. Máquina de Estados (Prueba de Vida y Control del Relé)
    switch (estado_medicion) {
        case CINTA_FINAL_OFF:
            timeout_hb = 1500; // Parpadeo lento (El sistema está en reposo)
            break;

        case CINTA_FINAL_IDLE:
            timeout_hb = 200;  // Parpadeo rápido (La cinta está encendida y esperando cajas)
            // (En el próximo paso conectaremos el disparo del HC-SR04 acá)
            break;

        case CINTA_FINAL_ESPERANDO_MEDICION:
            break;
            
        default:
            estado_medicion = CINTA_FINAL_OFF;
            break;
    }

    // 3. Heartbeat Visual (Test físico de que el Super Loop corre libremente)
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis(); 
        GPIO_ToggleHeartbeat(); // El LED de status de la placa debe titilar
    }

    // 4. Memoria del sensor para el próximo ciclo
    control_sensores[0].last_state = control_sensores[0].actual_state;
}