#include "app_cinta.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_servo.h"
#include <avr/io.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/common.h>
// Incluiríamos también tu mapeo de pines, asumo macros como PIN_ECHO, PORT_TRIGGER, etc.

// Variables estáticas (privadas al módulo) para mantener el estado
static eCintaState estado_actual = CINTA_IDLE;
static eCintaState estado_medicion = CINTA_IDLE;
static sColaCajas  fifo_cajas = { .head = 0, .tail = 0, .count = 0 };

// Variable para las secuecias de Heartbeat
static uint8_t timeout_hb = 0;
static uint8_t tick_hb = 0;

// Variables de temporización asíncrona
static uint32_t t_inicio_micros = 0;
static uint32_t t_inicio_millis = 0;

// Configuración de las salidas (Por defecto)
static uint8_t config_alturas_salida[3] = {ALTURA_CAJA_CHICA, ALTURA_CAJA_MEDIANA, ALTURA_CAJA_GRANDE};

static sEstadoServo control_servos[3];
static sSensores control_sensores[3] = {
    { .last_state = 1, .actual_state = 1 }, // Sensor S1
    { .last_state = 1, .actual_state = 1 }, // Sensor S2
    { .last_state = 1, .actual_state = 1 }  // Sensor S3
};

// Variables de la máquina de medición (S0)
static uint32_t t_inicio_micros = 0;

// Arreglo de 3 colas: 
// [0] Tramo S0->S1 | [1] Tramo S1->S2 | [2] Tramo S2->S3
static sColaCajas colas_zonas[3];

// Prototipos de funciones privadas
static void enqueue_caja(uint8_t altura_medida);
static sCaja dequeue_caja(void);
static sCaja* peek_caja(void);
static uint8_t calcular_destino(uint8_t altura);

void App_Cinta_Init(void) {
    estado_actual = CINTA_IDLE;
    estado_medicion = CINTA_IDLE;
    // Inicializar pines de sensores si no se hizo en main
    HAL_GPIO_SET_OUTPUT(DDRB, PIN_TRIGGER);
    HAL_GPIO_SET_INPUT(DDRB, PIN_ECHO);
    HAL_Servo_Init();
}

void App_Cinta_Task(void) {
    
    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Ingreso y Medición
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_OFF:
            // Aseguramos que el relé esté apagado
            HAL_GPIO_WRITE_LOW(PORTC, 0); 
            timeout_hb = 1500; // Seteamos el Heartbeat lento

            break;
        case CINTA_IDLE:
            // Si S0 detecta caja (Flanco)
            if (HAL_GPIO_READ(PIND, 2) == 0) { 
                HAL_GPIO_WRITE_HIGH(PORTB, 1); // Trigger ON
                t_inicio_micros = HAL_GetMicros();
                estado_medicion = CINTA_TRIGGER_ON;
            }
            timeout_hb = 500;
            break;
        case CINTA_TRIGGER_ON:
            // Evaluar delta de 10us para cortar el Trigger
            if ((HAL_GetMicros() - t_inicio_micros) >= 10) {
                HAL_GPIO_WRITE_LOW(PORTB, 1);
                t_inicio_micros = HAL_GetMicros(); // Reseteo para el timeout
                estado_medicion = CINTA_ESPERANDO_ECHO;
            }
            timeout_hb = 200;
            break;
        case CINTA_ESPERANDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) != 0) { // Pin ECHO en ALTO
                t_inicio_micros = HAL_GetMicros(); // Comienza a medir el pulso
                estado_medicion = CINTA_MIDIENDO_ECHO;
            } else if ((HAL_GetMicros() - t_inicio_micros) > 50000) {
                // Timeout de seguridad: Si el ECHO no sube en 50ms, aborto.
                estado_medicion = CINTA_IDLE;
            }
            break;
        case CINTA_MIDIENDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) == 0) { // Flanco bajada ECHO
                uint8_t altura_calc = Calcular_Altura(HAL_GetMicros() - t_inicio_micros);
                
                // Ingresa la caja al sistema físicamente (Tramo 0: hacia S1)
                Enqueue_Caja(&colas_zonas[0], altura_calc);
                
                estado_medicion = CINTA_IDLE; // Listo para la próxima caja
            }
            break;
        default:
            estado_medicion = CINTA_OFF;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 2: Polling de Zonas de Tránsito (Checkpoints)
    // ---------------------------------------------------------
    
    // Checkpoint S1 (Sensor PD3 activo en BAJO)

    for (uint8_t i = 0; i < 3; i++) {
        
        switch(i) {
            case 0:
                control_sensores[i].actual_state = LEER_SENSOR_S1();
                break;
            case 1:
                control_sensores[i].actual_state = LEER_SENSOR_S2();
                break;
            case 2:
                control_sensores[i].actual_state = LEER_SENSOR_S3();
                break;
        }

        // Lógica de Flanco y Máquina de Estados de Zona
        if (control_sensores[i].actual_state == 0 && control_sensores[i].last_state == 1 && !Cola_Vacia(&colas_zonas[i])) {
            
            sCaja* caja_actual = Peek_Caja(&colas_zonas[i]);
            
            // Asumiendo que los destinos válidos son 1, 2 y 3
            if (caja_actual->destino_salida == (i + 1)) { 
                HAL_Servo_SetAngle(i, 90); // Acá usamos la 'i' para el servo
                control_servos[i].tick_inicio = HAL_GetMillis();
                control_servos[i].en_movimiento = true;
            } else {
                // TRASVASE: Si no es para este sensor, va a la siguiente zona
                Enqueue_Caja(&colas_zonas[i + 1], caja_actual->altura); 
            }
            
            Dequeue_Caja(&colas_zonas[i]);
        }

        // Actualización de memoria
        control_sensores[i].last_state = control_sensores[i].actual_state;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Retracción Asíncrona de Servomotores
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        if (control_servos[i].en_movimiento) {
            if ((HAL_GetMillis() - control_servos[i].tick_inicio) >= 150) {
                HAL_Servo_SetAngle(i, 0); // Retraer brazo
                control_servos[i].en_movimiento = false;
            }
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 4: Polling asíncrono de la UART
    // ---------------------------------------------------------
    if (HAL_UART_RxDataAvailable()) {
        uint8_t dato = HAL_UART_RxRead();
        switch (dato){
            case 0x50:
                estado_medicion = CINTA_IDLE; 
                HAL_GPIO_WRITE_HIGH(PORTC, 0);
                // Resetear variables
                break;
            case 0x51:
                estado_medicion = CINTA_OFF; 
                HAL_GPIO_WRITE_LOW(PORTC, 0);
                break;
            default:
                break;
        }
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 5: Secuencia de Heartbeat
    // ---------------------------------------------------------
    if ((HAL_GetMillis() - tick_hb) >= timeout_hb) {
        tick_hb = HAL_GetMillis(); // Actualizacion de tick del heartbeat
        HAL_GPIO_TOGGLE(PORTB, 5);
    }
}

/*
void App_Cinta_Task(void) {
    
    // ---------------------------------------------------------
    // SUBSISTEMA 1: Máquina de Estados de Ingreso y Medición
    // ---------------------------------------------------------
    switch (estado_medicion) {
        case CINTA_IDLE:
            // Si S0 detecta caja (Flanco)
            if (HAL_GPIO_READ(PIND, 2) == 0) { 
                HAL_GPIO_WRITE_HIGH(PORTB, 1); // Trigger ON
                t_inicio_micros = HAL_GetMicros();
                estado_medicion = CINTA_TRIGGER_ON;
            }
            break;
        case CINTA_TRIGGER_ON:
            // Evaluar delta de 10us para cortar el Trigger
            if ((HAL_GetMicros() - t_inicio_micros) >= 10) {
                HAL_GPIO_WRITE_LOW(PORTB, 1);
                t_inicio_micros = HAL_GetMicros(); // Reseteo para el timeout
                estado_medicion = CINTA_ESPERANDO_ECHO;
            }
            break;

        case CINTA_ESPERANDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) != 0) { // Pin ECHO en ALTO
                t_inicio_micros = HAL_GetMicros(); // Comienza a medir el pulso
                estado_medicion = CINTA_MIDIENDO_ECHO;
            } else if ((HAL_GetMicros() - t_inicio_micros) > 50000) {
                // Timeout de seguridad: Si el ECHO no sube en 50ms, aborto.
                estado_medicion = CINTA_IDLE;
            }
            break;

        case CINTA_MIDIENDO_ECHO:
            if (HAL_GPIO_READ(PINB, 2) == 0) { // Flanco bajada ECHO
                uint8_t altura_calc = Calcular_Altura(HAL_GetMicros() - t_inicio_micros);
                
                // Ingresa la caja al sistema físicamente (Tramo 0: hacia S1)
                Enqueue_Caja(&colas_zonas[0], altura_calc);
                
                estado_medicion = CINTA_IDLE; // Listo para la próxima caja
            }
            break;
        default:
            estado_medicion = CINTA_IDLE;
            break;
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 2: Polling de Zonas de Tránsito (Checkpoints)
    // ---------------------------------------------------------
    
    // Checkpoint S1 (Sensor PD3 activo en BAJO)

    control_sensores[0].actual_state = LEER_SENSOR_S1();

    if (control_sensores[0].actual_state == 0 && control_sensores[0].last_state == 1  && !Cola_Vacia(&colas_zonas[0])) {
        
        sCaja* caja_actual = Peek_Caja(&colas_zonas[0]);
        
        if (caja_actual->destino_salida == 1) {
            HAL_Servo_SetAngle(SERVO_1, 90);
            control_servos[SERVO_1].tick_inicio = HAL_GetMillis();
            control_servos[SERVO_1].en_movimiento = true;
        } else {
            // No es para esta salida, avanza a la Zona 2
            Enqueue_Caja(&colas_zonas[1], caja_actual->altura);
        }
        
        // En ambos casos, abandona la Zona 1
        Dequeue_Caja(&colas_zonas[0]);
    }

    // Actualizamos el last_state
    control_sensores[0].last_state = control_sensores[0].actual_state;

    // Checkpoint S2
    if (HAL_GPIO_READ(PIND, 4) == 0 && !Cola_Vacia(&colas_zonas[1])) {
        
        sCaja* caja_actual = Peek_Caja(&colas_zonas[1]);
        
        if (caja_actual->destino_salida == 2) {
            HAL_Servo_SetAngle(SERVO_2, 90);
            control_servos[SERVO_2].tick_inicio = HAL_GetMillis();
            control_servos[SERVO_2].en_movimiento = true;
        } else {
            // No es para esta salida, avanza a la Zona 3
            Enqueue_Caja(&colas_zonas[2], caja_actual->altura);
        }
        
        // En ambos casos, abandona la Zona 2
        Dequeue_Caja(&colas_zonas[1]);
    }

    // Checkpoint S3
    if (HAL_GPIO_READ(PIND, 5) == 0 && !Cola_Vacia(&colas_zonas[2])) {
        
        sCaja* caja_actual = Peek_Caja(&colas_zonas[2]);
        
        if (caja_actual->destino_salida == 3) {
            HAL_Servo_SetAngle(SERVO_3, 90);
            control_servos[SERVO_3].tick_inicio = HAL_GetMillis();
            control_servos[SERVO_3].en_movimiento = true;
        }
        
        Dequeue_Caja(&colas_zonas[2]);
    }

    // ---------------------------------------------------------
    // SUBSISTEMA 3: Retracción Asíncrona de Servomotores
    // ---------------------------------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        if (control_servos[i].en_movimiento) {
            if ((HAL_GetMillis() - control_servos[i].tick_inicio) >= 150) {
                HAL_Servo_SetAngle(i, 0); // Retraer brazo
                control_servos[i].en_movimiento = false;
            }
        }
    }
}
*/