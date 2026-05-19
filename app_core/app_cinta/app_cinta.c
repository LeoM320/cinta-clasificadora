#include "app_cinta.h"
// Incluiríamos también tu mapeo de pines, asumo macros como PIN_ECHO, PORT_TRIGGER, etc.

// Prototipos de funciones privadas
static uint8_t Calcular_Destino(uint8_t altura);
static bool Cola_Vacia(_sColaCajas* cola);
static void Enqueue_Caja(_sColaCajas* cola, uint8_t altura_medida);
static _sCaja* Peek_Caja(_sColaCajas* cola);
static _sCaja Dequeue_Caja(_sColaCajas* cola);

// Variables estáticas (privadas al módulo) para mantener el estado
static _eCintaState estado_medicion = CINTA_IDLE;

// Variable para las secuecias de Heartbeat
static uint32_t timeout_hb = 0;
static uint32_t tick_hb = 0;

// Variable para mantener la velocidad global actualizada con cada caja
static uint32_t ultimo_delta_t_s0 = 1000;

// Variables para el calculo de velocidad
static uint32_t t_inicio_caja = 0;

// Configuración de las salidas (Por defecto)
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

// Variables para la altura de la caja
static uint16_t distancia_piso_cm = 0; 
static bool flag_calibrando = false;

// Arreglo de 3 colas: 
// [0] Tramo S0->S1 | [1] Tramo S1->S2 | [2] Tramo S2->S3
static _sColaCajas colas_zonas[3];

void App_Cinta_Init(void) {
    estado_medicion = CINTA_IDLE;
    HCSR04_Init();
    HAL_Servo_Init();

    // Inicializamos los 4 filtros anti-rebote
    for(uint8_t i = 0; i < 4; i++) {
        Debounce_Init(&filtro_sensores[i], 20, true); 
    }
}

void App_Cinta_Task(void) {
    HCSR04_Task();

    // 1. Leemos el hardware crudo de S0
    bool raw_s0 = HAL_GPIO_READ(PIND, 2);
    
    // 2. Lo pasamos por el filtro y guardamos el estado "limpio"
    control_sensores[0].actual_state = Debounce_Update(&filtro_sensores[0], raw_s0);

    // ---------------------------------------------------------
    // SUBSISTEMA 0: Estimación Cinemática (Velocidad S0)
    // ---------------------------------------------------------

    // Flanco de subida en S0 (La caja terminó de pasar)
    if (control_sensores[0].actual_state == 1 && control_sensores[0].last_state == 0) {

        // Actualizamos la velocidad global de la cinta
        ultimo_delta_t_s0 = HAL_GetMillis() - t_inicio_caja;
        
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
            HAL_GPIO_WRITE_LOW(PORTC, 0); 
            timeout_hb = 1500; 
            break;

        case CINTA_IDLE:
            if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1) { 
                t_inicio_caja = HAL_GetMillis(); // Inicia cronómetro velocidad
                if (HCSR04_Trigger()) {          // Dispara la medición asíncrona
                    estado_medicion = CINTA_ESPERANDO_MEDICION;
                }
            }
            timeout_hb = 500;
            break;

        case CINTA_ESPERANDO_MEDICION:
            // ¿El driver ya tiene el dato listo?
            if (HCSR04_IsDataReady()) { 
                uint16_t dist_medida_cm = HCSR04_GetDistance();

                if (flag_calibrando) {
                    distancia_piso_cm = dist_medida_cm;
                    flag_calibrando = false;
                } else {
                    // Restamos: Distancia al piso - Distancia a la caja
                    if (distancia_piso_cm > dist_medida_cm) {
                        uint8_t altura_caja = (uint8_t)(distancia_piso_cm - dist_medida_cm);
                        Enqueue_Caja(&colas_zonas[0], altura_caja);
                    }
                }
                estado_medicion = CINTA_IDLE; 
            } 
            // Timeout de respaldo de nuestra app (50ms)
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
        bool lectura_cruda = 1; // Asumimos 1 por defecto

        // 1. Lectura del hardware
        switch(i) {
            case 0: lectura_cruda = LEER_SENSOR_S1(); break;
            case 1: lectura_cruda = LEER_SENSOR_S2(); break;
            case 2: lectura_cruda = LEER_SENSOR_S3(); break;
        }

        // 2. Filtramos la señal y guardamos el resultado limpio
        control_sensores[i+1].actual_state = Debounce_Update(&filtro_sensores[i+1], lectura_cruda);

        // Solo evaluamos si hay cajas viajando en esta zona
        if (!Cola_Vacia(&colas_zonas[i])) {
            
            _sCaja* caja_esperada = Peek_Caja(&colas_zonas[i]);
            
            // A. Condición 1: Sensor Físico (Flanco de bajada)
            bool det_fisica = (control_sensores[i].actual_state == 0 && control_sensores[i].last_state == 1);
            
            // B. Condición 2: Sensor Virtual / ETA (Con margen de 1000ms)
            bool det_virtual = (HAL_GetMillis() >= (caja_esperada->tick_eta + 1000));
            
            // C. FUSIÓN: Si la ve el sensor físico O se vence el tiempo
            if (det_fisica || det_virtual) {

                // --- AVISO AL HMI ---
                // Si se activó por tiempo pero NO por sensor físico, el sensor está tapado/roto
                if (det_virtual && !det_fisica) {
                    // Le enviamos el código 0xE1 (Error de Sensor) a la interfaz de Qt
                    HAL_UART_TxWrite(0xE1);
                }

                // Evaluamos si es para esta salida
                if (caja_esperada->destino_salida == (i + 1)) { 
                    
                    // Calculamos la distancia hasta el centro de la caja
                    uint32_t dist_objetivo_mm = DISTANCIA_SENSOR_SERVO_MM + (LARGO_CAJA_MM / 2);
                    
                    // Calculamos los ms que debe esperar según la velocidad actual de la cinta
                    uint32_t delay_ms = (ultimo_delta_t_s0 * dist_objetivo_mm) / LARGO_CAJA_MM;
                    
                    // ¡Agendamos el golpe para el futuro!
                    control_servos[i].esperando_activacion = true;
                    control_servos[i].tick_programado = HAL_GetMillis() + delay_ms;
                    
                } else {
                    // TRASVASE: Avanza a la siguiente zona
                    if (i < 2) { 
                        Enqueue_Caja(&colas_zonas[i + 1], caja_esperada->altura); 
                    }
                }
                
                // CRÍTICO: La caja abandona la memoria de la zona actual inmediatamente
                Dequeue_Caja(&colas_zonas[i]);
            }
        }
        
        // Actualización de memoria del sensor físico
        control_sensores[i].last_state = control_sensores[i].actual_state;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Activación Retardada y Retracción de Servos
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        
        // A. Fase de Retardo: Esperando que la caja viaje esos 3 cm + la mitad de su cuerpo
        if (control_servos[i].esperando_activacion) {
            if (HAL_GetMillis() >= control_servos[i].tick_programado) {
                HAL_Servo_SetAngle(i, 90); 
                control_servos[i].tick_inicio = HAL_GetMillis();
                control_servos[i].en_movimiento = true;
                control_servos[i].esperando_activacion = false;
            }
        }

        // B. Fase de Retracción: Devuelve el servo a su posición de descanso a los 150ms
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
            case 0x50: // Encender Cinta
                estado_medicion = CINTA_IDLE; 
                HAL_GPIO_WRITE_HIGH(PORTC, 0); // Encender relé
                break;
            case 0x51: // Apagar Cinta
                estado_medicion = CINTA_OFF; 
                HAL_GPIO_WRITE_LOW(PORTC, 0);  // Apagar relé
                break;
            case 0x52: // Calibrar manual
                HAL_GPIO_WRITE_HIGH(PORTC, 0); // Encender relé
                flag_calibrando = true;
                HCSR04_Trigger(); // Pedimos medición a la librería
                t_inicio_caja = HAL_GetMillis(); // Reutilizamos variable para el timeout
                estado_medicion = CINTA_ESPERANDO_MEDICION;
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
bool Cola_Vacia(_sColaCajas* cola) {
    return (cola->count == 0);
}

// Inserta una nueva caja en la cola (Push)
static void Enqueue_Caja(_sColaCajas* cola, uint8_t altura_medida) {
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
static _sCaja* Peek_Caja(_sColaCajas* cola) {
    if (cola->count > 0) {
        // Retornamos la DIRECCIÓN de memoria (&) de la caja en el tail
        return &(cola->buffer[cola->tail]);
    }
    return NULL; // Retorna nulo si está vacía por seguridad
}

// Saca la caja de la primera posición (Pop)
static _sCaja Dequeue_Caja(_sColaCajas* cola) {
    _sCaja caja_salida = {0, 0}; // Caja vacía por defecto
    
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

void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada) {
    if (salida_idx < 3) {
        config_alturas_salida[salida_idx] = altura_asignada;
    }
}

