// ==========================================
// HAL/hal_timer.c
// ==========================================

/**
 * @file hal_timer.c
 * @brief Implementación del reloj base delegando en el Timer0 y la librería AVR-LibC.
 *
 * @details
 * La arquitectura subyacente es de 8 bits. Por lo tanto, leer una variable de 32 bits 
 * (`uint32_t`) como `system_millis` requiere 4 instrucciones de ensamblador. 
 * Si la interrupción del Timer0 se dispara en medio de estas instrucciones, la lectura 
 * devolverá un dato corrupto (*torn read*). Para solucionarlo, se emplea la macro 
 * `ATOMIC_BLOCK`, la cual deshabilita globalmente las interrupciones durante la copia 
 * y restaura el registro de estado `SREG` inmediatamente después.
 */

#include "../../include/hal_timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/** @brief Acumulador global de milisegundos. Marcado volatile por ser modificado en una ISR. */
volatile uint32_t system_millis = 0;

/** @brief Acumulador global de microsegundos base. Marcado volatile por ser modificado en una ISR. */
volatile uint32_t system_micros = 0;

void HAL_Timer0_Init(void)
{
    // Timer0: F_CPU = 16MHz, Prescaler 64 -> 1 tick = 4 microsegundos (us).
    // Modo CTC, TOP (OCR0A) = 249 -> 250 ticks * 4us = 1000us (1 milisegundo exacto).
    
    // Configura Modo CTC (Clear Timer on Compare Match)
    TCCR0A = (1 << WGM01);
    
    // Configura el Prescaler a 64 y arranca el timer
    TCCR0B = (1 << CS01) | (1 << CS00);
    
    // Fija el tope para la comparación
    OCR0A  = 249;
    
    // Habilita la interrupción por igualación de comparación A (Compare Match A)
    TIMSK0 |= (1 << OCIE0A);
}

/**
 * @brief Rutina de Servicio de Interrupción (ISR) del Timer0.
 * @details Se ejecuta estrictamente cada 1ms impuesta por el hardware.
 */
ISR(TIMER0_COMPA_vect)
{
    system_millis++;
    system_micros += 1000; // Sumamos los 1000us correspondientes al milisegundo transcurrido
}

uint32_t HAL_GetMillis(void)
{
    uint32_t ms;
    
    // Se extrae el valor global deshabilitando temporalmente las interrupciones
    // para garantizar una lectura atómica íntegra de la variable de 32 bits.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms = system_millis;
    }
    
    return ms;
}

uint32_t HAL_GetMicros(void)
{
    uint32_t us;
    uint8_t ticks;
    
    // Entramos en la sección crítica. 
    // Capturamos simultáneamente la base de tiempo (software) y el estado 
    // intermedio del hardware para evitar desfases.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        us = system_micros;
        ticks = TCNT0; // Leemos los ticks actuales del hardware (rango: 0 a 249)
    }
    
    // Interpolación matemática: 
    // Como prescaler = 64 y F_CPU = 16MHz, la resolución de un tick físico es de 4us.
    // Sumamos la fracción inconclusa de microsegundos a la base acumulada.
    return us + (ticks * 4);
}