#include "app_cinta.h"
#include "../hal/include/hal_uart.h"
#include "../drivers/hcsr04.h"
#include "../utils/debounce.h"

// Prototipos Privados
static uint8_t Calcular_Destino(uint8_t altura);
bool Cola_Vacia(sColaCajas* cola);
static void Enqueue_Caja(sColaCajas* cola, uint8_t altura_medida);
static sCaja* Peek_Caja(sColaCajas* cola);
static sCaja Dequeue_Caja(sColaCajas* cola);

static eCintaState estado_medicion = CINTA_IDLE;
static uint32_t timeout_hb = 0;
static uint32_t tick_hb = 0;
static uint32_t t_inicio_caja = 0;

static uint8_t config_alturas_salida[3] = {ALTURA_CAJA_CHICA, ALTURA_CAJA_MEDIANA, ALTURA_CAJA_GRANDE};
static sEstadoServo control_servos[3];

static sSensores control_sensores[4] = {
    {1, 1}, {1, 1}, {1, 1}, {1, 1}
};
static sColaCajas colas_zonas[3];

void App_Cinta_Init(void) {
    estado_medicion = CINTA_IDLE;
    // Ya no inicializamos pines directos acá, GPIO_Init() en main.c hace ese trabajo.
}

// NUEVA FUNCIÓN: API para que comandos.c controle la cinta
void App_Cinta_SetEstado(bool encendido) {
    if (encendido) {
        estado_medicion = CINTA_IDLE;
        HAL_GPIO_WRITE_HIGH(CINTA_PORT, CINTA_PIN);
    } else {
        estado_medicion = CINTA_OFF;
        HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
    }
}

// NUEVA FUNCIÓN: Configuración de salidas por HMI
void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada) {
    if (salida_idx < 3) {
        config_alturas_salida[salida_idx] = altura_asignada;
    }
}

void App_Cinta_Task(void) {
    // Lectura inicial de S0 (Trigger de entrada) usando macros oficiales
    control_sensores[0].actual_state = HAL_GPIO_READ(IR0_PIN_REG, IR0_PIN) != 0;

    // ---------------------------------------------------------
    // SUBSISTEMA 0: Estimación Cinemática (¡Tu lógica está perfecta!)
    // ---------------------------------------------------------
    if (control_sensores[0].actual_state == 1 && control_sensores[0].last_state == 0) {
        uint32_t delta_t_ms = HAL_GetMillis() - t_inicio_caja;
        uint32_t tiempo_viaje_estimado = delta_t_ms * 3; // (Distancia / Largo)
        uint32_t tick_esperado = HAL_GetMillis() + tiempo_viaje_estimado;
        
        if (!Cola_Vacia(&colas_zonas[0])) {
            uint8_t idx_ultima_caja = (colas_zonas[0].head == 0) ? (MAX_CAJAS_EN_CINTA - 1) : (colas_zonas[0].head - 1);
            colas_zonas[0].buffer[idx_ultima_caja].tick_eta = tick_esperado;
        }
    }
    
    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Medición (Corregida)
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_OFF:
            timeout_hb = 1500; 
            break;

        case CINTA_IDLE:
            if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1) { 
                t_inicio_caja = HAL_GetMillis();
                
                // DELEGAMOS EL TRABAJO: Le pedimos al driver que dispare
                HCSR04_Trigger();
                estado_medicion = CINTA_ESPERANDO_MEDICION;
            }
            timeout_hb = 500;
            break;

        case CINTA_ESPERANDO_MEDICION:
            // Preguntamos al driver asíncrono si ya terminó su trabajo
            if (HCSR04_IsDataReady()) {
                uint16_t distancia = HCSR04_GetDistance();
                
                // Asumiendo que el sensor está a 20 cm físico de la cinta:
                uint8_t altura_calc = (distancia < 20) ? (20 - distancia) : 0;
                
                Enqueue_Caja(&colas_zonas[0], altura_calc);
                estado_medicion = CINTA_IDLE; 
            }
            break;

        default:
            estado_medicion = CINTA_OFF;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 2: Polling de Zonas de Tránsito (Índices Corregidos)
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        switch(i) {
            case 0: control_sensores[1].actual_state = LEER_SENSOR_S1() != 0; break;
            case 1: control_sensores[2].actual_state = LEER_SENSOR_S2() != 0; break;
            case 2: control_sensores[3].actual_state = LEER_SENSOR_S3() != 0; break;
        }

        if (!Cola_Vacia(&colas_zonas[i])) {
            sCaja* caja_esperada = Peek_Caja(&colas_zonas[i]);
            
            // ¡BUG ARREGLADO! Ahora evaluamos [i+1], que es el sensor de salida real
            bool det_fisica = (control_sensores[i+1].actual_state == 0 && control_sensores[i+1].last_state == 1);
            bool det_virtual = (HAL_GetMillis() >= (caja_esperada->tick_eta + 1000));
            
            if (det_fisica || det_virtual) {
                if (caja_esperada->destino_salida == (i + 1)) { 
                    HAL_Servo_SetAngle(i, 90); 
                    control_servos[i].tick_inicio = HAL_GetMillis();
                    control_servos[i].en_movimiento = true;
                } else {
                    if (i < 2) { 
                        Enqueue_Caja(&colas_zonas[i + 1], caja_esperada->altura); 
                    }
                }
                Dequeue_Caja(&colas_zonas[i]);
            }
        }
        // ¡BUG ARREGLADO!
        control_sensores[i+1].last_state = control_sensores[i+1].actual_state;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Retracción Asíncrona de Servomotores
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        if (control_servos[i].en_movimiento) {
            if ((HAL_GetMillis() - control_servos[i].tick_inicio) >= 150) {
                HAL_Servo_SetAngle(i, 0); 
                control_servos[i].en_movimiento = false;
            }
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 4: Heartbeat Visual
    // ---------------------------------------------------------
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis();
        HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
    }

    control_sensores[0].last_state = control_sensores[0].actual_state;
}

// ---------------------------------------------------------
// FUNCIONES PRIVADAS (Colas y Matemáticas)
// ---------------------------------------------------------
static uint8_t Calcular_Destino(uint8_t altura) {
    // Usamos el arreglo dinámico en lugar de las macros fijas
    if (altura >= (config_alturas_salida[0] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[0] + TOLERANCIA_MEDICION)) return 1;
    if (altura >= (config_alturas_salida[1] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[1] + TOLERANCIA_MEDICION)) return 2;
    if (altura >= (config_alturas_salida[2] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[2] + TOLERANCIA_MEDICION)) return 3;
    return 0; 
}

bool Cola_Vacia(sColaCajas* cola) { return (cola->count == 0); }

static void Enqueue_Caja(sColaCajas* cola, uint8_t altura_medida) {
    if (cola->count < MAX_CAJAS_EN_CINTA) {
        cola->buffer[cola->head].altura = altura_medida;
        cola->buffer[cola->head].destino_salida = Calcular_Destino(altura_medida);
        cola->head = (cola->head + 1) % MAX_CAJAS_EN_CINTA;
        cola->count++;
    }
}

static sCaja* Peek_Caja(sColaCajas* cola) {
    if (cola->count > 0) return &(cola->buffer[cola->tail]);
    return NULL;
}

static sCaja Dequeue_Caja(sColaCajas* cola) {
    sCaja caja_salida = {0, 0, 0}; 
    if (cola->count > 0) {
        caja_salida = cola->buffer[cola->tail];
        cola->tail = (cola->tail + 1) % MAX_CAJAS_EN_CINTA;
        cola->count--;
    }
    return caja_salida;
}