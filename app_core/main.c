/**
 * @file main.c
 * @brief Orquestador para ejecutar la lógica autónoma de app_cinta.
 */

#include "config/hardware.h"
#include "config/gpio.h"

// Abstracciones de Hardware (HAL) requeridas por app_cinta.c
#include "hal/include/hal_timer.h"
#include "hal/include/hal_uart.h"

// Lógica de la aplicación
#include "app_cinta/app_cinta.h"

int main(void)
{
    // 1. SECCIÓN CRÍTICA: Desactivar interrupciones globales
    // Evita que una ISR intente acceder a hardware a medio configurar.
    HAL_DISABLE_INTERRUPTS();

    // 2. CONFIGURACIÓN ELÉCTRICA (Safe-State)
    // Fuerza todos los pines a su estado de reposo (ej. relé apagado, servos retraídos).
    GPIO_Init();

    // 3. INICIALIZACIÓN DE PERIFÉRICOS
    // El Timer0 es vital. Subsistemas enteros de app_cinta dependen de él: 
    // - Subsistema 0 (ETA de cajas) usa HAL_GetMillis().
    // - Subsistema 1 (Ultrasónico) usa HAL_GetMicros().
    // - Subsistema 3 (Retracción de Servos) usa HAL_GetMillis().
    HAL_Timer0_Init();
    
    // UART a 115200 baudios. Requerido por el Subsistema 4 para poder 
    // recibir los comandos 0x50 (Arrancar) y 0x51 (Detener).
    HAL_UART_Init(115200);

    // 4. INICIALIZACIÓN DE LA APLICACIÓN
    // Configura los pines específicos del sensor y levanta el Timer1 para los servos SG90.
    App_Cinta_Init();

    // 5. APERTURA DE HARDWARE
    // Habilitar interrupciones globales. Desde este momento, el Systick 
    // y el Ring Buffer de la UART comienzan a operar en segundo plano.
    HAL_ENABLE_INTERRUPTS();

    // 6. SUPER LOOP NO BLOQUEANTE
    while (1)
    {
        App_Cinta_Task();
    }

    return 0;
}