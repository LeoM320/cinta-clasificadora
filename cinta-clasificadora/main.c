#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/hardware.h"
#include "config/gpio.h"
#include "hal/hal_timer.h"
#include "hal/hal_servo.h"
#include "hal/hal_adc.h"          // <-- Tu nuevo módulo
#include "utils/temporizador.h"

int main(void)
{
    // Inicialización general
    GPIO_Init();
    HAL_Timer0_Init();
    HAL_Servo_Init();
    HAL_ADC_Init();               // <-- Arrancar el ADC

    sei();

    Temporizador timer_lectura;
    Temp_IniciarMS(&timer_lectura, 50); // Leemos el analógico 20 veces por segundo

    HAL_Servo_Enable(SERVO_1);

    while(1)
    {
        // Tarea: Lectura analógica y mapeo a servo
        if(Temp_Expiro(&timer_lectura))
        {
            // 1. Leer el potenciómetro (devuelve 0 a 1023)
            uint16_t valor_pote = HAL_ADC_Read(0); // 0 = Pin A0
            
            // 2. Mapear de (0-1023) a (0-180 grados)
            // Fórmula rápida usando matemática entera: (valor * 180) / 1023
            uint32_t calculo = ((uint32_t)valor_pote * 180) / 1023;
            uint8_t angulo = (uint8_t)calculo;
            
            // 3. Mover el servo en tiempo real
            HAL_Servo_SetAngle(SERVO_1, angulo);
            
            Temp_Reiniciar(&timer_lectura);
        }
        
        // Tus otras tareas siguen girando libremente por acá...
    }
}