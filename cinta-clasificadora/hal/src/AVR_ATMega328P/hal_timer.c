// ==========================================
// HAL/hal_timer.c
// ==========================================
#include "../include/hal_timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

volatile uint32_t system_millis = 0;
volatile uint32_t system_micros = 0;

void HAL_Timer0_Init(void)
{
    // Timer0: F_CPU = 16MHz, Prescaler 64 -> 4us por tick.
    // Modo CTC, TOP = 249 -> 250 ticks * 4us = 1000us (1ms).
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
    system_millis++;
    system_micros += 1000; // Sumamos los 1000us del milisegundo transcurrido
}

uint32_t HAL_GetMillis(void)
{
    uint32_t ms;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms = system_millis;
    }
    return ms;
}

uint32_t HAL_GetMicros(void)
{
    uint32_t us;
    uint8_t ticks;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        us = system_micros;
        ticks = TCNT0; // Leemos los ticks actuales (0 a 249)
    }
    // Cada tick vale 4 microsegundos
    return us + (ticks * 4);
}