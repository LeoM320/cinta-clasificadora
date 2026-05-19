#include "app_cinta.h"
// Incluiríamos también tu mapeo de pines, asumo macros como PIN_ECHO, PORT_TRIGGER, etc.

// Prototipos de funciones privadas
static uint8_t Calcular_Destino(uint8_t altura);
bool Cola_Vacia(sColaCajas* cola);
static void Enqueue_Caja(sColaCajas* cola, uint8_t altura_medida);
static sCaja* Peek_Caja(sColaCajas* cola);
static sCaja Dequeue_Caja(sColaCajas* cola);

// Variables estáticas (privadas al módulo) para mantener el estado
static eCintaState estado_medicion = CINTA_IDLE;

// Variable para las secuecias de Heartbeat
static uint32_t timeout_hb = 0;
static uint32_t tick_hb = 0;

// Variables para el calculo de velocidad
static uint32_t t_inicio_caja = 0;

// Variables de temporización asíncrona
static uint32_t t_inicio_micros = 0;
static uint32_t t_inicio_millis = 0;

// Configuración de las salidas (Por defecto)
static uint8_t config_alturas_salida[3] = {ALTURA_CAJA_CHICA, ALTURA_CAJA_MEDIANA, ALTURA_CAJA_GRANDE};

static sEstadoServo control_servos[3];
static sSensores control_sensores[4] = {
    { .last_state = 1, .actual_state = 1 }, // Sensor S0
    { .last_state = 1, .actual_state = 1 }, // Sensor S1
    { .last_state = 1, .actual_state = 1 }, // Sensor S2
    { .last_state = 1, .actual_state = 1 }  // Sensor S3
};

// Arreglo de 3 colas: 
// [0] Tramo S0->S1 | [1] Tramo S1->S2 | [2] Tramo S2->S3
static sColaCajas colas_zonas[3];

void App_Cinta_Init(void) {
    estado_medicion = CINTA_IDLE;
    HAL_GPIO_SET_OUTPUT(DDRB, PIN_TRIGGER);
    HAL_GPIO_SET_INPUT(DDRB, PIN_ECHO);
    HAL_Servo_Init();
}

void App_Cinta_Task(void) {
    // Leer el estado actual de S0 al inicio del ciclo
    control_sensores[0].actual_state = HAL_GPIO_READ(PIND, 2);

    // ---------------------------------------------------------
    // SUBSISTEMA 0: Estimación Cinemática (Velocidad S0)
    // ---------------------------------------------------------

    // Flanco de subida en S0 (La caja terminó de pasar)
    if (control_sensores[0].actual_state == 1 && control_sensores[0].last_state == 0) {
        
        // 1. ¿Cuánto tiempo tardó en pasar la caja por el sensor?
        uint32_t delta_t_ms = HAL_GetMillis() - t_inicio_caja;
        
        // 2. Factor geométrico: Distancia a S1 / Largo de la Caja
        // Asumimos que la distancia a S1 es el triple del largo de la caja
        uint8_t factor_distancia = 3; 
        
        // 3. Calculamos el tiempo de viaje estimado (matemática entera)
        uint32_t tiempo_viaje_estimado = delta_t_ms * factor_distancia;
        
        // 4. Calculamos el TICK EXACTO de llegada en el futuro
        uint32_t tick_esperado = HAL_GetMillis() + tiempo_viaje_estimado;
        
        // 5. Se lo asignamos a la caja que acabamos de meter en la zona 0
        if (!Cola_Vacia(&colas_zonas[0])) {
            // Retrocedemos 1 paso desde el 'head' para agarrar la última caja escrita.
            // Si el head es 0, damos la vuelta al final del Ring Buffer (MAX_CAJAS - 1)
            uint8_t idx_ultima_caja = (colas_zonas[0].head == 0) ? (MAX_CAJAS_EN_CINTA - 1) : (colas_zonas[0].head - 1);
            
            colas_zonas[0].buffer[idx_ultima_caja].tick_eta = tick_esperado;
        }
    }
    
    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Ingreso y Medición
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_OFF:
            HAL_GPIO_WRITE_LOW(PORTC, 0); // Asegurar relé apagado
            timeout_hb = 1500; 
            break;

        case CINTA_IDLE:
            // Detección de flanco descendente en S0 para iniciar medición
            if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1) { 
                HAL_GPIO_WRITE_HIGH(PORTB, 1); // Trigger ON
                t_inicio_caja = HAL_GetMillis();
                t_inicio_micros = HAL_GetMicros();
                estado_medicion = CINTA_TRIGGER_ON;
            }
            timeout_hb = 500;
            break;

        case CINTA_TRIGGER_ON:
            if ((HAL_GetMicros() - t_inicio_micros) >= 10) {
                HAL_GPIO_WRITE_LOW(PORTB, 1);
                t_inicio_micros = HAL_GetMicros(); 
                estado_medicion = CINTA_ESPERANDO_ECHO;
            }
            timeout_hb = 200;
            break;

        case CINTA_ESPERANDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) != 0) { 
                t_inicio_micros = HAL_GetMicros(); 
                estado_medicion = CINTA_MIDIENDO_ECHO;
            } else if ((HAL_GetMicros() - t_inicio_micros) > 50000) {
                estado_medicion = CINTA_IDLE;
            }
            break;

        case CINTA_MIDIENDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) == 0) { 
                uint8_t altura_calc = Calcular_Altura(HAL_GetMicros() - t_inicio_micros);
                Enqueue_Caja(&colas_zonas[0], altura_calc);
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
        // 1. Lectura del hardware
        switch(i) {
            case 0: control_sensores[i+1].actual_state = LEER_SENSOR_S1(); break;
            case 1: control_sensores[i+1].actual_state = LEER_SENSOR_S2(); break;
            case 2: control_sensores[i+1].actual_state = LEER_SENSOR_S3(); break;
        }

        // Solo evaluamos si hay cajas viajando en esta zona
        if (!Cola_Vacia(&colas_zonas[i])) {
            
            sCaja* caja_esperada = Peek_Caja(&colas_zonas[i]);
            
            // A. Condición 1: Sensor Físico (Flanco de bajada)
            bool det_fisica = (control_sensores[i].actual_state == 0 && control_sensores[i].last_state == 1);
            
            // B. Condición 2: Sensor Virtual / ETA (Con margen de 1000ms)
            bool det_virtual = (HAL_GetMillis() >= (caja_esperada->tick_eta + 1000));
            
            // C. FUSIÓN: Si la ve el sensor físico O se vence el tiempo
            if (det_fisica || det_virtual) {
                
                // Evaluamos si es para esta salida
                if (caja_esperada->destino_salida == (i + 1)) { 
                    HAL_Servo_SetAngle(i, 90); 
                    control_servos[i].tick_inicio = HAL_GetMillis();
                    control_servos[i].en_movimiento = true;
                } else {
                    // TRASVASE: Avanza a la siguiente zona
                    if (i < 2) { 
                        Enqueue_Caja(&colas_zonas[i + 1], caja_esperada->altura); 
                    }
                }
                
                // CRÍTICO: La caja abandona la memoria de la zona actual
                Dequeue_Caja(&colas_zonas[i]);
            }
        }
        
        // Actualización de memoria del sensor físico
        control_sensores[i].last_state = control_sensores[i].actual_state;
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
    // SUBSISTEMA 4: Polling asíncrono de la UART
    // ---------------------------------------------------------
    if (HAL_UART_RxDataAvailable()) {
        uint8_t dato = HAL_UART_RxRead();
        switch (dato) {
            case 0x50:
                estado_medicion = CINTA_IDLE; 
                HAL_GPIO_WRITE_HIGH(PORTC, 0); // Encender relé
                break;
            case 0x51:
                estado_medicion = CINTA_OFF; 
                HAL_GPIO_WRITE_LOW(PORTC, 0);  // Apagar relé
                break;
            default:
                break;
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 5: Secuencia de Heartbeat
    // ---------------------------------------------------------
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis(); // ¡Corregido con paréntesis!
        HAL_GPIO_TOGGLE(PORTB, 5);
    }

    // Actualizar el estado anterior de S0 al final de todo el ciclo
    control_sensores[0].last_state = control_sensores[0].actual_state;
}

static uint8_t Calcular_Destino(uint8_t altura) {
    if (altura >= (ALTURA_CAJA_CHICA - TOLERANCIA_MEDICION) && altura <= (ALTURA_CAJA_CHICA + TOLERANCIA_MEDICION)) {
        return 1;
    } else if (altura >= (ALTURA_CAJA_MEDIANA - TOLERANCIA_MEDICION) && altura <= (ALTURA_CAJA_MEDIANA + TOLERANCIA_MEDICION)) {
        return 2;
    } else if (altura >= (ALTURA_CAJA_GRANDE - TOLERANCIA_MEDICION) && altura <= (ALTURA_CAJA_GRANDE + TOLERANCIA_MEDICION)) {
        return 3;
    }
    return 0; // Descarte o tamaño no reconocido
}

// Verifica si la cola está vacía (retorna true si count es 0)
bool Cola_Vacia(sColaCajas* cola) {
    return (cola->count == 0);
}

// Inserta una nueva caja en la cola (Push)
static void Enqueue_Caja(sColaCajas* cola, uint8_t altura_medida) {
    // Condición de seguridad: Solo escribe si hay espacio
    if (cola->count < MAX_CAJAS_EN_CINTA) {
        // 1. Guardamos los datos en la posición actual del head
        cola->buffer[cola->head].altura = altura_medida;
        cola->buffer[cola->head].destino_salida = Calcular_Destino(altura_medida);
        
        // 2. Avanzamos el head de forma circular usando el módulo (%)
        cola->head = (cola->head + 1) % MAX_CAJAS_EN_CINTA;
        
        // 3. Aumentamos el contador de cajas en esta zona
        cola->count++;
    }
}

// Lee la caja en la primera posición sin sacarla (Peek)
static sCaja* Peek_Caja(sColaCajas* cola) {
    if (cola->count > 0) {
        // Retornamos la DIRECCIÓN de memoria (&) de la caja en el tail
        return &(cola->buffer[cola->tail]);
    }
    return NULL; // Retorna nulo si está vacía por seguridad
}

// Saca la caja de la primera posición (Pop)
static sCaja Dequeue_Caja(sColaCajas* cola) {
    sCaja caja_salida = {0, 0}; // Caja vacía por defecto
    
    if (cola->count > 0) {
        // 1. Copiamos la caja que está en el tail
        caja_salida = cola->buffer[cola->tail];
        
        // 2. Avanzamos el tail de forma circular
        cola->tail = (cola->tail + 1) % MAX_CAJAS_EN_CINTA;
        
        // 3. Disminuimos el contador
        cola->count--;
    }
    return caja_salida;
}

