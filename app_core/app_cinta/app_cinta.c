#include "app_cinta.h"

// Prototipos de funciones privadas
static uint8_t Calcular_Destino(uint8_t altura);
static bool Cola_Vacia(_sColaCajas* cola);
static void Enqueue_Caja(_sColaCajas* cola, uint8_t altura_medida);
static _sCaja* Peek_Caja(_sColaCajas* cola);
static _sCaja Dequeue_Caja(_sColaCajas* cola);

// Variables estáticas (privadas al módulo) para mantener el estado
static _eCintaState estado_medicion = CINTA_IDLE;

// Variable para las secuencias de Heartbeat
static uint32_t timeout_hb = 0;
static uint32_t tick_hb = 0;

// Variable para mantener la velocidad global actualizada con cada caja
static uint32_t ultimo_delta_t_s0 = 1000;

// Variables para el cálculo de velocidad y control de timeouts
static uint32_t t_inicio_caja = 0;

// Configuración dinámica de las alturas por salida (Modificable vía HMI/UART)
static uint8_t config_alturas_salida[3] = {ALTURA_CAJA_CHICA, ALTURA_CAJA_MEDIANA, ALTURA_CAJA_GRANDE};

static _sEstadoServo control_servos[3];
static _sSensores control_sensores[4] = {
    { .last_state = 1, .actual_state = 1 }, // Sensor S0
    { .last_state = 1, .actual_state = 1 }, // Sensor S1
    { .last_state = 1, .actual_state = 1 }, // Sensor S2
    { .last_state = 1, .actual_state = 1 }  // Sensor S3
};

// Creamos 4 instancias del filtro anti-rebote
static Debouncer_t filtro_sensores[4];

// Variables para la calibración automática del piso de la cinta
static uint16_t distancia_piso_cm = 0; 
static bool flag_calibrando = false;

// Arreglo de 3 colas de tránsito: [0] S0->S1 | [1] S1->S2 | [2] S2->S3
static _sColaCajas colas_zonas[3];

void App_Cinta_Init(void) {
    estado_medicion = CINTA_IDLE;
    HCSR04_Init();     // Inicialización de la FSM del driver del ultrasónico
    HAL_Servo_Init();

    // Inicializamos los 4 filtros anti-rebote (True por la lógica de Pull-Up)
    for(uint8_t i = 0; i < 4; i++) {
        Debounce_Init(&filtro_sensores[i], 20, true); 
    }
}

void App_Cinta_Task(void) {
    // Despachar el driver ultrasónico asíncrono
    HCSR04_Task();

    // Adquisición de S0 filtrada por el debouncer
    bool raw_s0 = HAL_GPIO_READ(PIND, 2);
    control_sensores[0].actual_state = Debounce_Update(&filtro_sensores[0], raw_s0);

    // ---------------------------------------------------------
    // SUBSISTEMA 0: Estimación Cinemática (Velocidad S0 y ETA)
    // ---------------------------------------------------------
    // Flanco de subida en S0 (La cola de la caja terminó de pasar)
    if (control_sensores[0].actual_state == 1 && control_sensores[0].last_state == 0) {

        // Calculamos cuánto tardó el cuerpo de la caja en cruzar el haz óptico
        uint32_t delta_t_ms = HAL_GetMillis() - t_inicio_caja;
        ultimo_delta_t_s0 = delta_t_ms; // Almacenamos velocidad global
        
        // Factor geométrico entero (Distancia S0->S1 es el triple del largo de la caja)
        uint8_t factor_distancia = 3; 
        uint32_t tiempo_viaje_estimado = delta_t_ms * factor_distancia;
        uint32_t tick_esperado = HAL_GetMillis() + tiempo_viaje_estimado;
        
        // Asignamos la marca de tiempo estimada (ETA) al registro encolado
        if (!Cola_Vacia(&colas_zonas[0])) {
            uint8_t idx_ultima_caja = (colas_zonas[0].head == 0) ? (MAX_CAJAS_EN_CINTA - 1) : (colas_zonas[0].head - 1);
            colas_zonas[0].buffer[idx_ultima_caja].tick_eta = tick_esperado;
        }
    }
    
    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Ingreso y Medición
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_OFF:
            HAL_GPIO_WRITE_LOW(PORTC, 0); // Corte de energía general del motor
            timeout_hb = 1500;            // Heartbeat en modo reposo (lento)
            break;

        case CINTA_IDLE:
            // Flanco de bajada en S0 (El frente de la caja empieza a obstruir)
            if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1) { 
                t_inicio_caja = HAL_GetMillis(); // Disparo del cronómetro cinematográfico
                if (HCSR04_Trigger()) {          // Disparo de ráfaga ultrasónica asíncrona
                    estado_medicion = CINTA_ESPERANDO_MEDICION;
                }
            }
            timeout_hb = 500; // Heartbeat en modo activo vacío (medio)
            break;

        case CINTA_ESPERANDO_MEDICION:
            // Consulta no bloqueante al driver de bajo nivel
            if (HCSR04_IsDataReady()) { 
                uint16_t dist_medida_cm = HCSR04_GetDistance();

                if (flag_calibrando) {
                    distancia_piso_cm = dist_medida_cm; // Sellar calibración de cinta vacía
                    flag_calibrando = false;
                } else {
                    // Verificación de resguardo geométrico
                    if (distancia_piso_cm > dist_medida_cm) {
                        uint8_t altura_caja = (uint8_t)(distancia_piso_cm - dist_medida_cm);
                        Enqueue_Caja(&colas_zonas[0], altura_caja);
                    }
                }
                estado_medicion = CINTA_IDLE; 
            } 
            // Timeout preventivo de la aplicación ante fallos físicos de eco
            else if ((HAL_GetMillis() - t_inicio_caja) > 50) {
                flag_calibrando = false;
                estado_medicion = CINTA_IDLE;
            }
            break;
            
        default:
            estado_medicion = CINTA_OFF;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 2: Polling de Zonas de Tránsito (Checkpoints)
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        bool lectura_cruda = true;

        // Mapeo dinámico del hardware
        switch(i) {
            case 0: lectura_cruda = LEER_SENSOR_S1(); break;
            case 1: lectura_cruda = LEER_SENSOR_S2(); break;
            case 2: lectura_cruda = LEER_SENSOR_S3(); break;
        }

        // Filtrado por debouncer indexado (Sensores S1, S2, S3 ocupan índices 1, 2, 3)
        control_sensores[i+1].actual_state = Debounce_Update(&filtro_sensores[i+1], lectura_cruda);

        // Si existen registros lógicos circulando en este tramo
        if (!Cola_Vacia(&colas_zonas[i])) {
            
            _sCaja* caja_esperada = Peek_Caja(&colas_zonas[i]);
            
            // Detección física por hardware (Flanco descendente en el checkpoint)
            bool det_fisica = (control_sensores[i+1].actual_state == 0 && control_sensores[i+1].last_state == 1);
            
            // Detección virtual por modelo (Vencimiento del ETA con 1000ms de gracia)
            bool det_virtual = (HAL_GetMillis() >= (caja_esperada->tick_eta + 1000));
            
            if (det_fisica || det_virtual) {

                // Reporte preventivo de mantenimiento si falló el haz físico y actuó el ETA
                if (det_virtual && !det_fisica) {
                    HAL_UART_TxWrite(0xE1);
                }

                // Si la caja pertenece a esta tolva de eyección
                if (caja_esperada->destino_salida == (i + 1)) { 
                    
                    // Cálculo dinámico de retardo mecánico para impactar en el CENTRO exacto
                    uint32_t dist_objetivo_mm = DISTANCIA_SENSOR_SERVO_MM + (LARGO_CAJA_MM / 2);
                    uint32_t delay_ms = (ultimo_delta_t_s0 * dist_objetivo_mm) / LARGO_CAJA_MM;
                    
                    // Agendar el disparo en el subsistema asíncrono de servos
                    control_servos[i].esperando_activacion = true;
                    control_servos[i].tick_programado = HAL_GetMillis() + delay_ms;
                    
                } else {
                    // Trasvase lógico: Empujar los datos hacia la cola del tramo subsiguiente
                    if (i < 2) { 
                        Enqueue_Caja(&colas_zonas[i + 1], caja_esperada->altura); 
                    }
                }
                
                // Consumo inmediato del buffer para limpiar la memoria circular
                Dequeue_Caja(&colas_zonas[i]);
            }
        }
        
        // Sincronización del historial del checkpoint para el siguiente ciclo
        control_sensores[i+1].last_state = control_sensores[i+1].actual_state;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Activación Retardada y Retracción de Servos
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        
        // Fase de Espera Cinemática finalizada: Ejecutar golpe mecánico
        if (control_servos[i].esperando_activacion) {
            if (HAL_GetMillis() >= control_servos[i].tick_programado) {
                HAL_Servo_SetAngle(i, 90); 
                control_servos[i].tick_inicio = HAL_GetMillis();
                control_servos[i].en_movimiento = true;
                control_servos[i].esperando_activacion = false;
            }
        }

        // Fase de Retracción Automática: Despejar la cinta transcurridos 150ms
        if (control_servos[i].en_movimiento) {
            if ((HAL_GetMillis() - control_servos[i].tick_inicio) >= 150) {
                HAL_Servo_SetAngle(i, 0); 
                control_servos[i].en_movimiento = false;
            }
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 4: Polling asíncrono de la UART
    // ---------------------------------------------------------
    if (HAL_UART_RxDataAvailable()) {
        uint8_t dato = HAL_UART_RxRead();
        switch (dato) {
            case 0x50: // Encender Planta
                estado_medicion = CINTA_IDLE; 
                HAL_GPIO_WRITE_HIGH(PORTC, 0); 
                break;
            case 0x51: // Parada de Emergencia / Apagar
                estado_medicion = CINTA_OFF; 
                HAL_GPIO_WRITE_LOW(PORTC, 0);  
                break;
            case 0x52: // Calibración dinámica del Piso a demanda
                HAL_GPIO_WRITE_HIGH(PORTC, 0); 
                flag_calibrando = true;
                HCSR04_Trigger(); 
                t_inicio_caja = HAL_GetMillis(); 
                estado_medicion = CINTA_ESPERANDO_MEDICION;
                break;
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 5: Secuencia de Heartbeat Dinámica
    // ---------------------------------------------------------
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis(); 
        HAL_GPIO_TOGGLE(PORTB, 5); // Toggles en pin LED de Arduino
    }

    // Sincronizar historial de S0
    control_sensores[0].last_state = control_sensores[0].actual_state;
}

static uint8_t Calcular_Destino(uint8_t altura) {
    // Clasificación adaptada a la matriz dinámica parametrizable desde Qt
    if (altura >= (config_alturas_salida[0] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[0] + TOLERANCIA_MEDICION)) {
        return 1;
    } else if (altura >= (config_alturas_salida[1] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[1] + TOLERANCIA_MEDICION)) {
        return 2;
    } else if (altura >= (config_alturas_salida[2] - TOLERANCIA_MEDICION) && altura <= (config_alturas_salida[2] + TOLERANCIA_MEDICION)) {
        return 3;
    }
    return 0; // Caja no clasificada
}

// Verifica si la cola está vacía (Encapsulada de forma privada)
static bool Cola_Vacia(_sColaCajas* cola) {
    return (cola->count == 0);
}

// Inserta una nueva caja en la cola (Push)
static void Enqueue_Caja(_sColaCajas* cola, uint8_t altura_medida) {
    if (cola->count < MAX_CAJAS_EN_CINTA) {
        cola->buffer[cola->head].altura = altura_medida;
        cola->buffer[cola->head].destino_salida = Calcular_Destino(altura_medida);
        cola->head = (cola->head + 1) % MAX_CAJAS_EN_CINTA;
        cola->count++;
    }
}

// Lee la caja en la primera posición sin extraerla (Peek)
static _sCaja* Peek_Caja(_sColaCajas* cola) {
    if (cola->count > 0) {
        return &(cola->buffer[cola->tail]);
    }
    return NULL;
}

// Extrae la caja de la primera posición (Pop)
static _sCaja Dequeue_Caja(_sColaCajas* cola) {
    _sCaja caja_salida = {0, 0, 0}; 
    if (cola->count > 0) {
        caja_salida = cola->buffer[cola->tail];
        cola->tail = (cola->tail + 1) % MAX_CAJAS_EN_CINTA;
        cola->count--;
    }
    return caja_salida;
}

// Implementación de la API de Configuración Pública Dinámica
void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada) {
    if (salida_idx < 3) {
        config_alturas_salida[salida_idx] = altura_asignada;
    }
}