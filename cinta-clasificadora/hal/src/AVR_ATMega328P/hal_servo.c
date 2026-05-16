/**
 * @file hal_servo.c
 * @brief Implementación segura del driver PWM multicanal.
 */

#include "../../include/hal_servo.h"
#include "../../../config/hardware.h"
#include "../../include/hal_gpio.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define MIN_PULSE_US  500
#define MAX_PULSE_US  2400

volatile uint16_t servo_ticks[3] = {3000, 3000, 3000}; 
volatile bool servo_habilitado[3] = {false, false, false}; 
volatile uint8_t servo_estado = 0;

void HAL_Servo_Init(void)
{
    HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);

    TCCR1A = 0;
    TCCR1B = (1 << CS11); 
    TIMSK1 |= (1 << OCIE1A);
    
    OCR1A = TCNT1 + 100; 
}

void HAL_Servo_SetPulse(uint8_t servo_id, uint16_t pulse_us)
{
    if (servo_id > 2) return;
    
    if (pulse_us < MIN_PULSE_US) pulse_us = MIN_PULSE_US;
    if (pulse_us > MAX_PULSE_US) pulse_us = MAX_PULSE_US;
    
    uint16_t ticks = pulse_us * 2;
    
    uint8_t sreg = SREG;
    cli();
    servo_ticks[servo_id] = ticks;
    SREG = sreg;
}

void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle)
{
    if (servo_id > 2) return;
    if (angle > 180) angle = 180; 

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
    
    if (servo_id == SERVO_1) HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    else if (servo_id == SERVO_2) HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    else if (servo_id == SERVO_3) HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
}

ISR(TIMER1_COMPA_vect)
{
    static uint16_t ticks_acumulados = 0;

    switch (servo_estado)
    {
        case 0:
            if (servo_habilitado[0]) {
                HAL_GPIO_WRITE_HIGH(SERVO1_PORT, SERVO1_PIN);
            }
            OCR1A += servo_ticks[0];
            ticks_acumulados = servo_ticks[0];
            servo_estado = 1;
            break;
            
        case 1:
            HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN); 
            if (servo_habilitado[1]) {
                HAL_GPIO_WRITE_HIGH(SERVO2_PORT, SERVO2_PIN);
            }
            OCR1A += servo_ticks[1];
            ticks_acumulados += servo_ticks[1];
            servo_estado = 2;
            break;
            
        case 2:
            HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
            if (servo_habilitado[2]) {
                HAL_GPIO_WRITE_HIGH(SERVO3_PORT, SERVO3_PIN);
            }
            OCR1A += servo_ticks[2];
            ticks_acumulados += servo_ticks[2];
            servo_estado = 3;
            break;
            
        case 3:
            HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
            OCR1A += (40000 - ticks_acumulados);
            servo_estado = 0;
            break;
    }
}