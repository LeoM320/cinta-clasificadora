#include <stdint.h>
#include <stdbool.h>
#include "heartbeat.h"
#include "temporizador.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"

static Temporizador timer_led;
static uint8_t current_sequence = 0;
static uint8_t bit_index = 0;

void Heartbeat_Init(uint32_t bit_duration_ms, uint8_t initial_sequence) {
    current_sequence = initial_sequence;
    bit_index = 0;
    
    // Inicializar el reloj de transición
    Temp_IniciarMS(&timer_led, bit_duration_ms);
    
    // Aseguramos que el pin sea salida y arranque apagado
    HAL_GPIO_SET_OUTPUT(STATUS_LED_DDR, STATUS_LED_PIN);
    HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
}

void Heartbeat_SetSequence(uint8_t new_sequence) {
    current_sequence = new_sequence;
}

void Heartbeat_Task(void) {
    if (Temp_Expiro(&timer_led)) {
        
        // Extraer el estado lógico del bit actual.
        // Se lee de izquierda a derecha (MSB a LSB), por ende: 7 - bit_index
        bool bit_state = (current_sequence & (1 << (7 - bit_index))) != 0;
        
        // Aplicar el estado físico al LED
        if (bit_state) {
            HAL_GPIO_WRITE_HIGH(STATUS_LED_PORT, STATUS_LED_PIN);
        } else {
            HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
        }
        
        // Avanzar el índice de forma circular (0 a 7 y vuelve a 0)
        bit_index = (bit_index + 1) % 8;
        
        Temp_Reiniciar(&timer_led);
    }
}