/**
 * @file main.c
 * @brief Programa principal para pruebas de hardware.
 * @author LeoM320
 * @date 14/05/2026
 */ 

#include <avr/io.h>
#include <util/delay.h>         // Necesaria para los retardos
#include "config/hardware.h"    // Tu mapa de pines
#include "config/gpio.h"        // Tu lógica de inicialización
#include "hal/hal_gpio.h"       // Tus macros de control

/**
 * @brief Punto de entrada del programa.
 * Inicializa el hardware y ejecuta un blink infinito en el LED de estado.
 */
int main(void)
{
    // 1. Inicializar todos los pines según la configuración
    GPIO_Init();

    // 2. Bucle infinito
    while(1)
    {
		HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
		_delay_ms(500);
        // Encender LED usando tu macro HAL y el nombre de hardware.h
        //HAL_GPIO_WRITE_HIGH(STATUS_LED_PORT, STATUS_LED_PIN);
        //_delay_ms(500);
        
        // Apagar LED
        //HAL_GPIO_WRITE_LOW(STATUS_LED_PORT, STATUS_LED_PIN);
        //_delay_ms(500);
        
        /* * Prueba extra: Podés usar el Toggle para simplificarlo:
         * HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
         * _delay_ms(500);
         */
    }
}