/**
 * @file hal_servo.c
 * @brief Implementación de la máquina de estados del driver PWM secuencial.
 *
 * @details
 * La máquina de estados opera dentro de la ISR del Timer1. 
 * Con F_CPU a 16 MHz y prescaler de 8, cada unidad de `TCNT1` equivale a 0.5 us.
 * Por lo tanto, un ciclo de refresco estándar de 20 ms (50 Hz) requiere exactamente 
 * 40,000 ticks. La rutina atiende un servo tras otro, disparando los pulsos en cascada, 
 * y en el último estado programa un retardo largo (40000 - ticks utilizados) para 
 * mantener la frecuencia base rigurosamente constante.
 */

#include "../../include/hal_servo.h"
#include "../../../config/hardware.h"
#include "../../include/hal_gpio.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/** @brief Límite inferior físico de seguridad para el pulso (0 grados). */
#define MIN_PULSE_US  500
/** @brief Límite superior físico de seguridad para el pulso (180 grados). */
#define MAX_PULSE_US  2400

/** @brief Array de longitudes de pulso convertidas a ticks de hardware. 
 *  Se declaran volatile por ser recursos compartidos con la ISR. */
volatile uint16_t servo_ticks[3] = {3000, 3000, 3000}; 

/** @brief Flags de control de energía para decidir si se emite físicamente la señal en el pin. */
volatile bool servo_habilitado[3] = {false, false, false}; 

/** @brief Índice del estado actual en el secuenciador de pulsos de la ISR. */
volatile uint8_t servo_estado = 0;

void HAL_Servo_Init(void)
{
    // Aseguramos que los pines comiencen en estado inactivo (LOW)
    HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);

    // Modo Normal (el contador desborda en 0xFFFF de forma libre).
    TCCR1A = 0;
    
    // Prescaler /8: Arranca el temporizador.
    TCCR1B = (1 << CS11); 
    
    // Habilitar Interrupción de Comparación A.
    TIMSK1 |= (1 << OCIE1A);
    
    // Forzar un disparo cercano inicial.
    OCR1A = TCNT1 + 100; 
}

void HAL_Servo_SetPulse(uint8_t servo_id, uint16_t pulse_us)
{
    if (servo_id > 2) return;
    
    // Clamping de seguridad para no destruir el motor
    if (pulse_us < MIN_PULSE_US) pulse_us = MIN_PULSE_US;
    if (pulse_us > MAX_PULSE_US) pulse_us = MAX_PULSE_US;
    
    // Conversión de microsegundos a ticks (1us = 2 ticks a F_CPU 16MHz/8)
    uint16_t ticks = pulse_us * 2;
    
    // Protección de Sección Crítica: 
    // Una escritura en variable de 16 bits requiere dos instrucciones en AVR.
    // Inhabilitamos interrupciones localmente para asegurar integridad de datos.
    uint8_t sreg = SREG;
    cli();
    servo_ticks[servo_id] = ticks;
    SREG = sreg; // Restaura las interrupciones a su estado anterior.
}

void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle)
{
    if (servo_id > 2) return;
    if (angle > 180) angle = 180; 

    // Aproximación lineal matemática: Pulse = Min + Angle * ((Max - Min) / 180)
    // Con Max=2400 y Min=500 -> Rango útil = 1900us. 1900 / 180 = ~10.55 us/grado.
    // La fórmula `angle * 10 + angle / 2` equivale algorítmicamente a `angle * 10.5` en enteros.
    uint16_t pulse_us = MIN_PULSE_US + ((uint16_t)angle * 10) + (angle / 2);
    
    HAL_Servo_SetPulse(servo_id, pulse_us);
}

void HAL_Servo_Enable(uint8_t servo_id)
{
    if (servo_id > 2) return;
    servo_habilitado[servo_id] = true;
}

void HAL_Servo_Disable(uint8_t servo_id)
{
    if (servo_id > 2) return;
    
    servo_habilitado[servo_id] = false;
    
    // Se fuerza físicamente el pin a masa para asegurar el corte inmediato
    if (servo_id == SERVO_1) HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    else if (servo_id == SERVO_2) HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    else if (servo_id == SERVO_3) HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
}

/**
 * @brief Rutina principal de generación PWM secuencial (Máquina de estados).
 * 
 * La interrupción agenda la próxima alarma sumando al registro de comparación 
 * (`OCR1A`) los ticks requeridos para el pulso del servo activo.
 */
ISR(TIMER1_COMPA_vect)
{
    static uint16_t ticks_acumulados = 0;

    switch (servo_estado)
    {
        case 0:
            // Inicio de Trama: Disparamos Servo 1 (si corresponde)
            if (servo_habilitado[0]) {
                HAL_GPIO_WRITE_HIGH(SERVO1_PORT, SERVO1_PIN);
            }
            OCR1A += servo_ticks[0];           // Agendamos el tiempo de corte
            ticks_acumulados = servo_ticks[0]; // Reseteamos contador maestro de trama
            servo_estado = 1;
            break;
            
        case 1:
            // Apagamos Servo 1, Disparamos Servo 2
            HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN); 
            if (servo_habilitado[1]) {
                HAL_GPIO_WRITE_HIGH(SERVO2_PORT, SERVO2_PIN);
            }
            OCR1A += servo_ticks[1];
            ticks_acumulados += servo_ticks[1];
            servo_estado = 2;
            break;
            
        case 2:
            // Apagamos Servo 2, Disparamos Servo 3
            HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
            if (servo_habilitado[2]) {
                HAL_GPIO_WRITE_HIGH(SERVO3_PORT, SERVO3_PIN);
            }
            OCR1A += servo_ticks[2];
            ticks_acumulados += servo_ticks[2];
            servo_estado = 3;
            break;
            
        case 3:
            // Apagamos Servo 3. Trama activa finalizada.
            HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
            
            // Calculamos el tiempo de espera restante para completar el periodo de 20ms (40,000 ticks)
            // Esto asegura que la base de tiempo global se respete.
            OCR1A += (40000 - ticks_acumulados);
            servo_estado = 0;
            break;
    }
}