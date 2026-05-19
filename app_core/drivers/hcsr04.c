#include "hcsr04.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_timer.h"

#define TRIGGER_PULSE_US       10UL
#define HCSR04_TIMEOUT_US      24000UL /**< Timeout equivalente a ~400 cm (fuera de rango) */
#define SOUND_SPEED_DIVISOR    58      /**< Divisor empírico para convertir microsegundos a cm */

typedef enum {
    HCSR04_STATE_IDLE,
    HCSR04_STATE_TRIGGER_HIGH,
    HCSR04_STATE_WAIT_ECHO_START,
    HCSR04_STATE_WAIT_ECHO_END
} HCSR04_State_t;

static HCSR04_State_t current_state = HCSR04_STATE_IDLE;
static uint32_t timer_reference = 0;
static uint16_t last_distance_cm = 0;
static bool data_ready = false;

void HCSR04_Init(void) {
    HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
    current_state = HCSR04_STATE_IDLE;
    data_ready = false;
    last_distance_cm = 0;
}

bool HCSR04_Trigger(void) {
    if (current_state != HCSR04_STATE_IDLE) {
        return false; // Medición actualmente en curso, ignorar nuevo disparo
    }
    
    HAL_GPIO_WRITE_HIGH(TRIGGER_PORT, TRIGGER_PIN);
    timer_reference = HAL_GetMicros();
    current_state = HCSR04_STATE_TRIGGER_HIGH;
    return true;
}

void HCSR04_Task(void) {
    uint32_t current_time = HAL_GetMicros();
    
    switch (current_state) {
        case HCSR04_STATE_IDLE:
            break;
            
        case HCSR04_STATE_TRIGGER_HIGH:
            if ((current_time - timer_reference) >= TRIGGER_PULSE_US) {
                HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN);
                timer_reference = current_time;
                current_state = HCSR04_STATE_WAIT_ECHO_START;
            }
            break;
            
        case HCSR04_STATE_WAIT_ECHO_START:
            if (HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
                // El pin Echo subió a ALTO, iniciamos conteo de tiempo de vuelo
                timer_reference = current_time;
                current_state = HCSR04_STATE_WAIT_ECHO_END;
            } else if ((current_time - timer_reference) > HCSR04_TIMEOUT_US) {
                // Timeout: el sensor nunca respondió (cable desconectado o falla)
                current_state = HCSR04_STATE_IDLE; 
            }
            break;
            
        case HCSR04_STATE_WAIT_ECHO_END:
            if (!HAL_GPIO_READ(ECHO_PIN_REG, ECHO_PIN)) {
                // El pin Echo bajó a BAJO, calculamos distancia
                uint32_t echo_length = current_time - timer_reference;
                last_distance_cm = (uint16_t)(echo_length / SOUND_SPEED_DIVISOR);
                data_ready = true;
                current_state = HCSR04_STATE_IDLE;
            } else if ((current_time - timer_reference) > HCSR04_TIMEOUT_US) {
                // Objeto fuera de rango o pérdida de eco
                current_state = HCSR04_STATE_IDLE; 
            }
            break;
    }
}

uint16_t HCSR04_GetDistance(void) {
    data_ready = false; // Se limpia la bandera al leer el dato
    return last_distance_cm;
}

bool HCSR04_IsDataReady(void) {
    return data_ready;
}