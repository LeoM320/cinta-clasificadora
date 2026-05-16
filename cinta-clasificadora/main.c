/**
 * @file main.c
 * @brief Prueba de máquina de estados para el control de Servos con temporizadores.
 * @author LeoM320
 * @date 15/05/2026
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/hal_gpio.h"
#include "hal/hal_timer.h"
#include "hal/hal_servo.h"
#include "utils/temporizador.h"

int main(void)
{
    // ==========================================
    // 1. Inicialización
    // ==========================================
    GPIO_Init();
    HAL_Timer0_Init();
    HAL_Servo_Init();

    sei(); // Habilitar interrupciones globales

    // ==========================================
    // 2. Configuración de Temporizadores
    // ==========================================
    Temporizador timer_led;
    Temporizador timer_secuencia;

    // El LED parpadea cada 250ms para demostrar que el micro no se bloquea
    Temp_IniciarMS(&timer_led, 250);
    
    // Arrancamos la secuencia del servo casi instantáneamente
    Temp_IniciarMS(&timer_secuencia, 10);

    uint8_t paso_secuencia = 0;

    // ==========================================
    // 3. Super Loop (Máquina de Estados)
    // ==========================================
    while(1)
    {
        // --- TAREA 1: Heartbeat (Latido de vida) ---
        if(Temp_Expiro(&timer_led))
        {
            HAL_GPIO_TOGGLE(STATUS_LED_PORT, STATUS_LED_PIN);
            Temp_Reiniciar(&timer_led);
        }

        // --- TAREA 2: Secuencia del Servo 1 ---
        if(Temp_Expiro(&timer_secuencia))
        {
            switch(paso_secuencia)
            {
                case 0:
                    // Habilitar el motor y llevarlo a 0 grados
                    HAL_Servo_Enable(SERVO_1);
                    HAL_Servo_SetAngle(SERVO_1, 0);
                    
                    Temp_IniciarMS(&timer_secuencia, 1000); // Esperar 1 segundo
                    paso_secuencia = 1;
                    break;
                    
                case 1:
                    // Mover al centro (90 grados)
                    HAL_Servo_SetAngle(SERVO_1, 90);
                    
                    Temp_IniciarMS(&timer_secuencia, 1000); // Esperar 1 segundo
                    paso_secuencia = 2;
                    break;
                    
                case 2:
                    // Mover al extremo (180 grados)
                    HAL_Servo_SetAngle(SERVO_1, 180);
                    
                    Temp_IniciarMS(&timer_secuencia, 1000); // Esperar 1 segundo
                    paso_secuencia = 3;
                    break;
                    
                case 3:
                    // Deshabilitar el motor de forma segura
                    // Acá vas a notar que el motor pierde fuerza de retención
                    HAL_Servo_Disable(SERVO_1);
                    
                    Temp_IniciarMS(&timer_secuencia, 2000); // Dejarlo "suelto" 2 segundos
                    paso_secuencia = 0; // Volver a empezar
                    break;
            }
        }
    }
}