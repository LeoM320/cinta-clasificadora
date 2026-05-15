/**
 * @file main.c
 * @brief Programa principal para pruebas de temporización por software (Polling).
 * @author LeoM320
 * @date 14/05/2026
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/hal_gpio.h"
#include "hal/hal_timer.h"
#include "utils/temporizador.h"

int main(void)
{
    // ==========================================
    // 1. Inicialización de Hardware
    // ==========================================
    GPIO_Init();        
    HAL_Timer0_Init();  

    sei(); // ¡Fundamental habilitar interrupciones!

    // ==========================================
    // 2. Configuración de Tareas (Timers de Software)
    // ==========================================
    Temporizador timer_lento;
    Temporizador timer_rapido;
    Temporizador timer_micro; // Nuevo timer para us

    // Tarea 1: Onda lenta (500ms -> 1 Hz)
    Temp_IniciarMS(&timer_lento, 500);
    
    // Tarea 2: Onda media (100ms -> 5 Hz)
    Temp_IniciarMS(&timer_rapido, 100);

    // Tarea 3: Onda ultra rápida (500us -> 1 kHz)
    Temp_IniciarUS(&timer_micro, 500);

    // ==========================================
    // 3. Bucle Principal (Super Loop)
    // ==========================================
    while(1)
    {
        // Tarea 1: STATUS_LED
        if(Temp_Expiro(&timer_lento))
        {
            HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
            Temp_Reiniciar(&timer_lento);
        }

        // Tarea 2: SERVO1
        if(Temp_Expiro(&timer_rapido))
        {
            HAL_GPIO_TOGGLE(SERVO1_PORT, SERVO1_PIN);
            Temp_Reiniciar(&timer_rapido);
        }

        // Tarea 3: SERVO2 (Prueba de micros)
        if(Temp_Expiro(&timer_micro))
        {
            HAL_GPIO_TOGGLE(SERVO2_PORT, SERVO2_PIN);
            Temp_Reiniciar(&timer_micro);
        }
    }
}