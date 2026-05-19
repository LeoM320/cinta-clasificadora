// ==========================================
// hal/src/AVR_ATMega328P/hal_servo.c
// ==========================================

/**
 * @file hal_servo.c
 * @brief Implementación de la máquina de estados del driver PWM secuencial.
 */

#include "../../include/hal_servo.h"
#include "../../include/hal_gpio.h"
#include "../../../config/hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define MIN_PULSE_US  500
#define MAX_PULSE_US  2400

// Registros de control para el perfil de velocidad
volatile uint16_t servo_current_ticks[3] = {3000, 3000, 3000}; // Ancho de pulso actual emitido
volatile uint16_t servo_target_ticks[3]  = {3000, 3000, 3000}; // Ancho de pulso objetivo final
volatile uint16_t servo_step[3]          = {0, 0, 0};          // Ticks a incrementar/decrementar por frame (20ms)

volatile bool servo_habilitado[3] = {false, false, false}; 
volatile uint8_t servo_estado = 0;

void HAL_Servo_Init(void)
{
    HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);

    TCCR1A = 0;
    TCCR1B = (1 << CS11); // Prescaler /8 (1 tick = 0.5us a 16MHz)
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
    // Salto instantáneo: Objetivo y actual son iguales, el paso se anula
    servo_target_ticks[servo_id]  = ticks;
    servo_current_ticks[servo_id] = ticks;
    servo_step[servo_id]          = 0;
    SREG = sreg;
}

void HAL_Servo_SetAngle(uint8_t servo_id, uint8_t angle)
{
    if (servo_id > 2) return;
    if (angle > 180) angle = 180; 

    uint16_t pulse_us = MIN_PULSE_US + ((uint16_t)angle * 10) + (angle / 2);
    HAL_Servo_SetPulse(servo_id, pulse_us);
}

void HAL_Servo_SetAngleSmooth(uint8_t servo_id, uint8_t target_angle, uint16_t transition_time_ms)
{
    if (servo_id > 2) return;
    if (target_angle > 180) target_angle = 180;

    uint16_t pulse_us = MIN_PULSE_US + ((uint16_t)target_angle * 10) + (target_angle / 2);
    uint16_t target_ticks = pulse_us * 2;

    uint8_t sreg = SREG;
    cli(); // Sección Crítica
    
    uint16_t current_ticks = servo_current_ticks[servo_id];
    
    // Si piden menos de 20ms (1 frame) o ya estamos en posición, saltar al instante
    if (transition_time_ms < 20 || current_ticks == target_ticks) {
        servo_target_ticks[servo_id]  = target_ticks;
        servo_current_ticks[servo_id] = target_ticks;
        servo_step[servo_id]          = 0;
    } else {
        servo_target_ticks[servo_id] = target_ticks;
        
        // Calcular la distancia total en ticks
        uint16_t diff = (target_ticks > current_ticks) ? 
                        (target_ticks - current_ticks) : 
                        (current_ticks - target_ticks);
                        
        // Frames disponibles (a razón de 50 frames por segundo / 1 frame = 20ms)
        uint16_t frames = transition_time_ms / 20; 
        
        uint16_t step = diff / frames;
        if (step == 0) step = 1; // Seguridad matemática
        
        servo_step[servo_id] = step;
    }
    
    SREG = sreg;
}

void HAL_Servo_Enable(uint8_t servo_id) {
    if (servo_id > 2) return;
    servo_habilitado[servo_id] = true;
}

void HAL_Servo_Disable(uint8_t servo_id) {
    if (servo_id > 2) return;
    servo_habilitado[servo_id] = false;
    
    if (servo_id == SERVO_1) HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN);
    else if (servo_id == SERVO_2) HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
    else if (servo_id == SERVO_3) HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
}

// ==========================================
// MÁQUINA DE ESTADOS E INTERPOLACIÓN (ISR)
// ==========================================

ISR(TIMER1_COMPA_vect)
{
    static uint16_t ticks_acumulados = 0;

    switch (servo_estado)
    {
        case 0:
            if (servo_habilitado[0]) HAL_GPIO_WRITE_HIGH(SERVO1_PORT, SERVO1_PIN);
            OCR1A += servo_current_ticks[0];
            ticks_acumulados = servo_current_ticks[0];
            servo_estado = 1;
            break;
            
        case 1:
            HAL_GPIO_WRITE_LOW(SERVO1_PORT, SERVO1_PIN); 
            if (servo_habilitado[1]) HAL_GPIO_WRITE_HIGH(SERVO2_PORT, SERVO2_PIN);
            OCR1A += servo_current_ticks[1];
            ticks_acumulados += servo_current_ticks[1];
            servo_estado = 2;
            break;
            
        case 2:
            HAL_GPIO_WRITE_LOW(SERVO2_PORT, SERVO2_PIN);
            if (servo_habilitado[2]) HAL_GPIO_WRITE_HIGH(SERVO3_PORT, SERVO3_PIN);
            OCR1A += servo_current_ticks[2];
            ticks_acumulados += servo_current_ticks[2];
            servo_estado = 3;
            break;
            
        case 3:
            HAL_GPIO_WRITE_LOW(SERVO3_PORT, SERVO3_PIN);
            OCR1A += (40000 - ticks_acumulados); // Completar ciclo base de 20ms
            servo_estado = 0;
            
            // --- CÁLCULO DEL PERFIL DE TRANSICIÓN (SWEEP) ---
            // Se ejecuta al final del frame para preparar el pulso del siguiente ciclo
            for (uint8_t i = 0; i < 3; i++) {
                if (servo_step[i] > 0) {
                    
                    if (servo_current_ticks[i] < servo_target_ticks[i]) {
                        // Incremento con protección de desbordamiento
                        if ((servo_target_ticks[i] - servo_current_ticks[i]) <= servo_step[i]) {
                            servo_current_ticks[i] = servo_target_ticks[i];
                            servo_step[i] = 0; // Llegó a destino
                        } else {
                            servo_current_ticks[i] += servo_step[i];
                        }
                        
                    } else if (servo_current_ticks[i] > servo_target_ticks[i]) {
                        // Decremento con protección de desbordamiento (Integer Underflow)
                        if ((servo_current_ticks[i] - servo_target_ticks[i]) <= servo_step[i]) {
                            servo_current_ticks[i] = servo_target_ticks[i];
                            servo_step[i] = 0; // Llegó a destino
                        } else {
                            servo_current_ticks[i] -= servo_step[i];
                        }
                    }
                }
            }
            break;
    }
}