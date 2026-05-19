/**
 * @file heartbeat.c
 * @brief Implementación del secuenciador visual (Heartbeat).
 *
 * @details
 * La implementación utiliza el módulo `temporizador.h` para el manejo del tiempo
 * de forma no bloqueante y la capa de abstracción de hardware (HAL) para manipular
 * los pines del GPIO, garantizando la portabilidad del código.
 */

#include <stdint.h>
#include <stdbool.h>
#include "heartbeat.h"
#include "temporizador.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"

/** @brief Temporizador de software para el control de la duración de cada bit. */
static Temporizador timer_led;

/** @brief Registro que contiene el patrón visual activo. */
static uint8_t current_sequence = 0;

/** @brief Índice del bit que se está evaluando actualmente (0 a 7). */
static uint8_t bit_index = 0;

/**
 * @brief Inicializa los parámetros de la secuencia y asegura el estado inicial del GPIO.
 * 
 * @param[in] bit_duration_ms Duración de cada "tick" visual.
 * @param[in] initial_sequence Patrón de arranque.
 */
void Heartbeat_Init(uint32_t bit_duration_ms, uint8_t initial_sequence) {
    current_sequence = initial_sequence;
    bit_index = 0;
    
    // Inicializar el reloj de transición
    Temp_IniciarMS(&timer_led, bit_duration_ms);
    
    // Aseguramos que el pin sea salida y arranque apagado
    HAL_GPIO_SET_OUTPUT(STATUS_LED_DDR, STATUS_LED_PIN);
    HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
}

/**
 * @brief Sobrescribe el patrón de parpadeo activo.
 * 
 * @param[in] new_sequence Nuevo patrón binario.
 */
void Heartbeat_SetSequence(uint8_t new_sequence) {
    current_sequence = new_sequence;
}

/**
 * @brief Evalúa el bit actual y actualiza el hardware si el temporizador expiró.
 */
void Heartbeat_Task(void) {
    if (Temp_Expiro(&timer_led)) {
        
        // Extraer el estado lógico del bit actual.
        // Se desplaza la máscara para leer de izquierda a derecha (MSB a LSB).
        // Ej: Cuando bit_index == 0, se evalúa el bit 7.
        bool bit_state = (current_sequence & (1 << (7 - bit_index))) != 0;
        
        // Aplicar el estado físico al LED mediante la HAL
        if (bit_state) {
            HAL_GPIO_WRITE_HIGH(STATUS_LED_PORT, STATUS_LED_PIN);
        } else {
            HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
        }
        
        // Avanzar el índice de forma circular (0 a 7 y vuelve a 0)
        bit_index = (bit_index + 1) % 8;
        
        // Reiniciar el temporizador para el siguiente bit
        Temp_Reiniciar(&timer_led);
    }
}