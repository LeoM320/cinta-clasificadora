/**
 * @file hal_adc.c
 * @brief Implementación del driver del ADC por Polling.
 */

#include "hal_adc.h"
#include <avr/io.h>

void HAL_ADC_Init(void)
{
    // REFS0 en 1: Usar AVCC (5V) como voltaje de referencia máximo.
    // Los bits ADMUX para el canal arrancan en 0 (Canal A0 por defecto).
    ADMUX = (1 << REFS0);

    // ADEN: Habilita el ADC
    // ADPS2, ADPS1, ADPS0 en 1: Prescaler de 128 (16MHz / 128 = 125 KHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t HAL_ADC_Read(uint8_t channel)
{
    // 1. Asegurar que el canal pedido sea válido (0 a 7) usando una máscara de bits
    channel &= 0x07;
    
    // 2. Limpiar los 3 bits inferiores de ADMUX (que guardan el canal viejo) 
    // y aplicarle el canal nuevo con un OR lógico.
    ADMUX = (ADMUX & 0xF8) | channel;
    
    // 3. Disparar la conversión encendiendo el bit ADSC (ADC Start Conversion)
    ADCSRA |= (1 << ADSC);
    
    // 4. Esperar a que el hardware termine. 
    // El hardware apaga el bit ADSC automáticamente cuando termina de convertir.
    while (ADCSRA & (1 << ADSC));
    
    // 5. Retornar el valor de 16 bits. 
    // AVR-GCC es inteligente y lee los registros ADCL y ADCH en el orden correcto.
    return ADC;
}