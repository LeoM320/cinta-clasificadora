/**
 * @file gpio.c
 * @brief Implementación de la inicialización de los pines (GPIO) del sistema.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 * * Este archivo contiene la función principal para configurar la dirección y 
 * el estado inicial de todos los pines físicos utilizados en la cinta clasificadora,
 * haciendo uso de la capa de abstracción de hardware (HAL_GPIO).
 */

#include "gpio.h"
#include "hardware.h"
#include "../hal/hal_gpio.h"

/**
 * @brief Inicializa los puertos de entrada y salida del microcontrolador.
 * * @details Esta función configura los siguientes periféricos:
 * - HC-SR04: Pin Trigger (Salida en Bajo) y Pin Echo (Entrada sin pull-up).
 * - Servomotores: Pines de control PWM configurados como Salidas en Bajo.
 * - LED de estado: Configurado como Salida en Bajo.
 * - TCRT5000: Sensores infrarrojos configurados como Entradas.
 * - Cinta: Motor principal configurado como Salida en Bajo.
 * * @note Deshabilita el buffer digital en los pines analógicos (DIDR0)
 * correspondientes a los sensores IR para reducir el consumo de energía.
 * * @return void No devuelve ningún valor.
 */
void GPIO_Init(void)
{
    // Trigger HC-SR04
    HAL_GPIO_WRITE_LOW(TRIGGER_PORT, TRIGGER_PIN); // 1. Estado inicial BAJO
    HAL_GPIO_SET_OUTPUT(TRIGGER_DDR, TRIGGER_PIN); // 2. Configurar como SALIDA

    // Echo HC-SR04 (Entrada sin pull-up)
    HAL_GPIO_WRITE_LOW(ECHO_PORT, ECHO_PIN);       // 1. Desactivar pull-up
    HAL_GPIO_SET_INPUT(ECHO_DDR, ECHO_PIN);        // 2. Configurar como ENTRADA

    // Servos
    HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO1_DDR, SERVO1_PIN);
    
    HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO2_DDR, SERVO2_PIN);
    
    HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
    HAL_GPIO_SET_OUTPUT(SERVO3_DDR, SERVO3_PIN);
    
    // LED de estado
    HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
    HAL_GPIO_SET_OUTPUT(STATUS_LED_DDR, STATUS_LED_PIN);

    // TCRT5000 como entradas sin pull-up
    HAL_GPIO_WRITE_LOW(IR0_PORT, IR0_PIN);
    HAL_GPIO_SET_INPUT(IR0_DDR, IR0_PIN);
    
    HAL_GPIO_WRITE_LOW(IR1_PORT, IR1_PIN);
    HAL_GPIO_SET_INPUT(IR1_DDR, IR1_PIN);
    
    HAL_GPIO_WRITE_LOW(IR2_PORT, IR2_PIN);
    HAL_GPIO_SET_INPUT(IR2_DDR, IR2_PIN);
    
    HAL_GPIO_WRITE_LOW(IR3_PORT, IR3_PIN);
    HAL_GPIO_SET_INPUT(IR3_DDR, IR3_PIN);
    
    // Deshabilitar buffer digital en pines analógicos para ahorrar energía
    DIDR0 |= (1 << IR0_PIN) | (1 << IR1_PIN) | (1 << IR2_PIN) | (1 << IR3_PIN);

    // Cinta transportadora
    HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
    HAL_GPIO_SET_OUTPUT(CINTA_DDR, CINTA_PIN);
}