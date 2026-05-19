/**
 * @file hcsr04.c
 * @brief Implementación de la FSM y cálculos físicos para el HC-SR04.
 *
 * @details
 * La lógica se basa en 4 estados asíncronos. Se incluye un mecanismo de 
 * "Timeout" de seguridad fijado en 24,000 microsegundos. Este valor no es 
 * aleatorio: a la velocidad del sonido (~343 m/s), el sonido tarda unos 
 * 23.3 ms en ir y volver a un objeto situado a 4 metros de distancia (el 
 * límite físico del HC-SR04). Si el eco demora más que esto, se asume pérdida 
 * de señal o falla de hardware y la FSM se reinicia de manera segura.
 */

#include "hcsr04.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_timer.h"

/** @brief Duración obligatoria por hoja de datos (datasheet) del pulso de disparo. */
#define TRIGGER_PULSE_US       10UL

/** @brief Tiempo máximo de espera (24ms). Protege al firmware de bloqueos infinitos. */
#define HCSR04_TIMEOUT_US      24000UL 

/** 
 * @brief Divisor empírico para la conversión espacio-tiempo.
 * Velocidad del sonido = 340 m/s = 0.034 cm/us.
 * Distancia = (Tiempo * 0.034) / 2 (ida y vuelta).
 * Simplificación matemática entera: Tiempo / 58 = Distancia en cm.
 */
#define SOUND_SPEED_DIVISOR    58      

/**
 * @brief Estados internos de la máquina finita del HC-SR04.
 */
typedef enum {
    HCSR04_STATE_IDLE,             /**< Reposo, esperando orden de Trigger */
    HCSR04_STATE_TRIGGER_HIGH,     /**< Manteniendo el pin Trigger en ALTO por 10us */
    HCSR04_STATE_WAIT_ECHO_START,  /**< Esperando a que el hardware suba el pin Echo */
    HCSR04_STATE_WAIT_ECHO_END     /**< Contando el tiempo de vuelo hasta que Echo baje */
} HCSR04_State_t;

/** @brief Variable de estado actual de la FSM. */
static HCSR04_State_t current_state = HCSR04_STATE_IDLE;

/** @brief Marca de tiempo (timestamp) en microsegundos para cálculos delta. */
static uint32_t timer_reference = 0;

/** @brief Buffer de retención para la última lectura validada en centímetros. */
static uint16_t last_distance_cm = 0;

/** @brief Bandera indicadora de disponibilidad de un dato no leído. */
static bool data_ready = false;

void HCSR04_Init(void) {
    HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
    current_state = HCSR04_STATE_IDLE;
    data_ready = false;
    last_distance_cm = 0;
}

bool HCSR04_Trigger(void) {
    if (current_state != HCSR04_STATE_IDLE) {
        return false; // Medición actualmente en curso, ignorar nuevo disparo para evitar corrupción
    }
    
    // Iniciar pulso excitador
    HAL_GPIO_WRITE_HIGH(TRIGGER_PORT, TRIGGER_PIN);
    timer_reference = HAL_GetMicros();
    current_state = HCSR04_STATE_TRIGGER_HIGH;
    return true;
}

void HCSR04_Task(void) {
    uint32_t current_time = HAL_GetMicros();
    
    switch (current_state) {
        case HCSR04_STATE_IDLE:
            // Sin operaciones, conservando ciclos de CPU.
            break;
            
        case HCSR04_STATE_TRIGGER_HIGH:
            if ((current_time - timer_reference) >= TRIGGER_PULSE_US) {
                // Finaliza el pulso de 10us exactos exigidos por el sensor
                HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
                timer_reference = current_time;
                current_state = HCSR04_STATE_WAIT_ECHO_START;
            }
            break;
            
        case HCSR04_STATE_WAIT_ECHO_START:
            if (HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
                // El pin Echo subió a ALTO, el módulo emitió el tren de pulsos (Burst)
                // Iniciamos conteo estricto del tiempo de vuelo
                timer_reference = current_time;
                current_state = HCSR04_STATE_WAIT_ECHO_END;
            } else if ((current_time - timer_reference) > HCSR04_TIMEOUT_US) {
                // Timeout crítico: El sensor nunca respondió (cable desconectado o hardware dañado)
                current_state = HCSR04_STATE_IDLE; 
            }
            break;
            
        case HCSR04_STATE_WAIT_ECHO_END:
            if (!HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
                // El pin Echo volvió a BAJO, el sonido regresó.
                // Cálculo físico de la distancia limitando el uso de punto flotante.
                uint32_t echo_length = current_time - timer_reference;
                last_distance_cm = (uint16_t)(echo_length / SOUND_SPEED_DIVISOR);
                data_ready = true;
                current_state = HCSR04_STATE_IDLE;
            } else if ((current_time - timer_reference) > HCSR04_TIMEOUT_US) {
                // Objeto fuera de rango (mayor a 4m) o el eco se dispersó.
                // Restablecemos para permitir futuros intentos sin bloquear el sistema.
                current_state = HCSR04_STATE_IDLE; 
            }
            break;
    }
}

uint16_t HCSR04_GetDistance(void) {
    data_ready = false; // "Clear on read" (Se limpia la bandera al leer el dato de forma atómica)
    return last_distance_cm;
}

bool HCSR04_IsDataReady(void) {
    return data_ready;
}