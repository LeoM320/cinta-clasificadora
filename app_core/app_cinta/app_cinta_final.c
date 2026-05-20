#include "app_cinta_final.h"
#include "app/comandos.h" // Necesario para llamar a Comandos_EnviarLog
#include <stdio.h>        // Necesario para snprintf

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

static uint16_t ultima_distancia_cm = 0; // Caché para la telemetría de la PC

static uint32_t t_disparo_eco = 0; // Cronómetro exclusivo para el timeout del HC-SR04

// Buffer global estático para armar los mensajes de Log sin saturar el Stack
static char msg_buffer[60];

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
void App_CintaFinal_Init(void) {
    estado_medicion = CINTA_FINAL_OFF; 
    
    HCSR04_Init();     
    HAL_Servo_Init();

    // ESTE ES EL BLOQUE QUE LES DA FUERZA A LOS SERVOS
    for(uint8_t i = 0; i < 3; i++) {
        HAL_Servo_Enable(i);       // Habilita la señal PWM
        HAL_Servo_SetAngle(i, 90);  // Los traba en 90 grados (Estado de reposo / retraídos)
    }

    for(uint8_t i = 0; i < 4; i++) {
        Debounce_Init(&filtro_sensores[i], 20, true); 
    }
    
    Comandos_EnviarLog("Sistema Iniciado. Servos listos.");
}

// ============================================================================
// API DE CONTROL PÚBLICA (Llamadas desde comandos.c)
// ============================================================================

void App_CintaFinal_SetEstado(bool encender) {
    if (encender) {
        estado_medicion = CINTA_FINAL_IDLE; 
        Comandos_EnviarLog("Cinta ENCENDIDA");
    } else {
        estado_medicion = CINTA_FINAL_OFF; 
        Comandos_EnviarLog("Cinta APAGADA");
    }
    // Despachamos la orden física a la capa BSP (Hardware)
    GPIO_SetCinta(encender); 
}

void App_CintaFinal_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada) {
    if (salida_idx < 3) {
        config_alturas_salida[salida_idx] = altura_asignada;
        snprintf(msg_buffer, sizeof(msg_buffer), "Cfg: Salida %u -> Altura %u cm", salida_idx + 1, altura_asignada);
        Comandos_EnviarLog(msg_buffer);
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
// MÁQUINA DE ESTADOS PRINCIPAL (VERSIÓN 2: CINEMÁTICA Y ULTRASÓNICO)
// ============================================================================

void App_CintaFinal_Task(void) {
    // 1. Lectura del hardware base (Sensor S0) y filtrado anti-rebote
    bool raw_s0 = GPIO_LeerSensor(0);
    control_sensores[0].actual_state = Debounce_Update(&filtro_sensores[0], raw_s0);

    // ---------------------------------------------------------
    // SUBSISTEMA 0: Estimación Cinemática (Velocidad S0)
    // ---------------------------------------------------------
    // Flanco de subida en S0 (La parte trasera de la caja terminó de pasar)
    if (control_sensores[0].actual_state == 1 && control_sensores[0].last_state == 0) {
        // Calculamos cuánto tardó el cuerpo de la caja en cruzar el haz óptico
        uint32_t delta_t_ms = HAL_GetMillis() - t_inicio_caja;
        ultimo_delta_t_s0 = delta_t_ms; // Almacenamos velocidad global
        
        // LOG: Informamos la velocidad calculada
        snprintf(msg_buffer, sizeof(msg_buffer), "S0: Fin caja. dt=%lu ms", delta_t_ms);
        Comandos_EnviarLog(msg_buffer);
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Ingreso y Medición
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_FINAL_OFF:
            timeout_hb = 1500; 
            break;

        case CINTA_FINAL_IDLE:
            // Flanco de bajada en S0 (El frente de la caja acaba de tocar el láser)
            if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1) { 
                t_inicio_caja = HAL_GetMillis(); // Empezamos a contar el tiempo
                Comandos_EnviarLog("S0: Ingreso detectado.");
                
                // En vez de disparar ya, pasamos al estado de espera
                estado_medicion = CINTA_FINAL_RETARDO_CENTRADO; 
            }
            timeout_hb = 200;
            break;

        case CINTA_FINAL_RETARDO_CENTRADO:
            // Esperamos 500ms a que la cinta arrastre la caja justo debajo del sensor
            if ((HAL_GetMillis() - t_inicio_caja) > 500) {
                if (HCSR04_Trigger()) {  
                    t_disparo_eco = HAL_GetMillis(); // <--- ARRANCAMOS EL CRONÓMETRO DEL ECO ACÁ
                    estado_medicion = CINTA_FINAL_ESPERANDO_MEDICION;
                }
            }
            break;

        case CINTA_FINAL_ESPERANDO_MEDICION:
            if (HCSR04_IsDataReady()) { 
                ultima_distancia_cm = HCSR04_GetDistance(); 

                if (ALTURA_SENSOR_PISO_CM > ultima_distancia_cm) {
                    uint8_t altura_caja = (uint8_t)(ALTURA_SENSOR_PISO_CM - ultima_distancia_cm);
                    uint8_t destino = Calcular_Destino(altura_caja);
                    
                    // LOG DETALLADO: Informamos destino y los límites del intervalo matemático
                    if (destino > 0) {
                        uint8_t min_h = config_alturas_salida[destino - 1] - TOLERANCIA_MEDICION_CM;
                        uint8_t max_h = config_alturas_salida[destino - 1] + TOLERANCIA_MEDICION_CM;
                        snprintf(msg_buffer, sizeof(msg_buffer), "Medicion: Alt=%u cm -> Dest: Z%u [%u-%u cm]", 
                                 altura_caja, destino, min_h, max_h);
                    } else {
                        snprintf(msg_buffer, sizeof(msg_buffer), "Medicion: Alt=%u cm -> Dest: DESCARTE", 
                                 altura_caja);
                    }
                    Comandos_EnviarLog(msg_buffer);
                    
                    Enqueue_Caja(&colas_zonas[0], altura_caja); 
                } else {
                    // LOG: Lectura ruidosa o caja inexistente
                    snprintf(msg_buffer, sizeof(msg_buffer), "Medicion vacia: Dist=%u cm", ultima_distancia_cm);
                    Comandos_EnviarLog(msg_buffer);
                }
                estado_medicion = CINTA_FINAL_IDLE; 
            } 
            // <--- EVALUAMOS EL TIMEOUT DE 50ms CON LA NUEVA VARIABLE
            else if ((HAL_GetMillis() - t_disparo_eco) > 50) { 
                Comandos_EnviarLog("Medicion: TIMEOUT del sensor HC-SR04");
                estado_medicion = CINTA_FINAL_IDLE;
            }
        break;
            
        default:
            estado_medicion = CINTA_FINAL_OFF;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 2: Polling de Zonas de Tránsito (Checkpoints S1, S2, S3)
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        // 1. Leemos el hardware dinámicamente (S1 es el índice 1, S2 el 2, etc.)
        bool lectura_cruda = GPIO_LeerSensor(i + 1); 
        control_sensores[i+1].actual_state = Debounce_Update(&filtro_sensores[i+1], lectura_cruda);

        // 2. Evaluamos la zona solo si hay una caja viajando en ella
        if (!Cola_Vacia(&colas_zonas[i])) {
            
            sCajaFinal* caja_esperada = Peek_Caja(&colas_zonas[i]);
            
            // Detección Física: Flanco descendente en S1, S2 o S3
            bool det_fisica = (control_sensores[i+1].actual_state == 0 && control_sensores[i+1].last_state == 1);
            
            if (det_fisica) { // AHORA SOLO ENTRA SI EL SENSOR IR SE TAPA REALMENTE

                if (caja_esperada->destino_salida == (i + 1)) { 
                    // ES SU DESTINO: Calculamos el delay para golpearla justo en el centro
                    uint32_t dist_objetivo_mm = DISTANCIA_SENSOR_SERVO_MM + (LARGO_CAJA_ESTIMADO_MM / 2);
                    uint32_t delay_ms = (ultimo_delta_t_s0 * dist_objetivo_mm) / LARGO_CAJA_ESTIMADO_MM;
                    
                    // LOG: Confirmación de objetivo y retardo
                    snprintf(msg_buffer, sizeof(msg_buffer), "Z%u: OBJETIVO! Programando golpe en %lu ms", i+1, delay_ms);
                    Comandos_EnviarLog(msg_buffer);
                    
                    // Programamos el servomotor de forma asíncrona
                    control_servos[i].esperando_activacion = true;
                    control_servos[i].tick_programado = HAL_GetMillis() + delay_ms;
                    
                } else {
                    // NO ES SU DESTINO: Empujamos la caja hacia la memoria de la siguiente zona
                    if (i < 2) { 
                        snprintf(msg_buffer, sizeof(msg_buffer), "Z%u: Paso de largo. Traspasando a Z%u", i+1, i+2);
                        Comandos_EnviarLog(msg_buffer);
                        Enqueue_Caja(&colas_zonas[i + 1], caja_esperada->altura); 
                    } else {
                        // Pasó el último sensor y no era de ninguna zona (descarte)
                        Comandos_EnviarLog("Z3: Caja de descarte detectada al final.");
                    }
                }
                
                // Sacamos la caja de la memoria de la zona actual
                Dequeue_Caja(&colas_zonas[i]);
            }
        }
        
        // Sincronizamos la memoria del sensor para el próximo ciclo
        control_sensores[i+1].last_state = control_sensores[i+1].actual_state;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Activación Retardada y Retracción de Servos
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        
        // Ejecutar golpe mecánico (0 grados) si llegó el momento programado
        if (control_servos[i].esperando_activacion) {
            if (HAL_GetMillis() >= control_servos[i].tick_programado) {
                HAL_Servo_SetAngle(i, 0); 
                control_servos[i].tick_inicio = HAL_GetMillis();
                control_servos[i].en_movimiento = true;
                control_servos[i].esperando_activacion = false;
                
                // LOG: Acción de empuje (Golpe)
                snprintf(msg_buffer, sizeof(msg_buffer), "Z%u: Servo EXTENDIDO (0 grados)", i + 1);
                Comandos_EnviarLog(msg_buffer);
            }
        }

        // Retracción automática (90 grados): Despejar la cinta 300ms después del golpe
        if (control_servos[i].en_movimiento) {
            if ((HAL_GetMillis() - control_servos[i].tick_inicio) >= 300) {
                HAL_Servo_SetAngle(i, 90); 
                control_servos[i].en_movimiento = false;
                
                // LOG: Acción de retorno (Reposo)
                snprintf(msg_buffer, sizeof(msg_buffer), "Z%u: Servo RETRAIDO (90 grados)", i + 1);
                Comandos_EnviarLog(msg_buffer);
            }
        }
    }

    // ---------------------------------------------------------
    // Heartbeat Visual y Actualización de Memoria
    // ---------------------------------------------------------
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis(); 
        GPIO_ToggleHeartbeat(); 
    }

    control_sensores[0].last_state = control_sensores[0].actual_state;
}

// ============================================================================
// API DE TELEMETRÍA PARA COMANDOS.C (Lectura Segura)
// ============================================================================
uint16_t App_CintaFinal_GetDistancia(void) {
    return ultima_distancia_cm;
}

uint8_t App_CintaFinal_GetIRPack(void) {
    uint8_t pack = 0;
    // Empaquetamos los estados FILTRADOS, no los crudos del hardware
    if (control_sensores[0].actual_state) pack |= (1 << 0);
    if (control_sensores[1].actual_state) pack |= (1 << 1);
    if (control_sensores[2].actual_state) pack |= (1 << 2);
    if (control_sensores[3].actual_state) pack |= (1 << 3);
    return pack;
}