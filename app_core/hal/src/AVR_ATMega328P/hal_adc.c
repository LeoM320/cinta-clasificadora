/**
 * @file hal_adc.c
 * @brief Implementación lógica del controlador ADC por Polling.
 *
 * @details
 * El diseño de este módulo optimiza la frecuencia de reloj del ADC. 
 * Según las especificaciones técnicas (datasheet) de la arquitectura AVR, 
 * para obtener la resolución completa de 10 bits, el reloj del ADC debe 
 * operar estrictamente entre 50 KHz y 200 KHz. 
 * Esta implementación asume una frecuencia principal (F_CPU) de 16 MHz y 
 * aplica un pre-escalador de 128, resultando en un reloj de ADC de 125 KHz, 
 * lo cual representa el punto operativo ideal.
 */

#include "../../include/hal_adc.h"
#include <avr/io.h>

void HAL_ADC_Init(void)
{
    // REFS0 en 1: Usar AVCC como voltaje de referencia máximo (típicamente 5V).
    // Los bits ADMUX para el canal arrancan en 0 (Canal A0 seleccionado por defecto).
    ADMUX = (1 << REFS0);

    // Configuración del registro de control A (ADCSRA):
    // - ADEN: Enciende físicamente el módulo ADC interno.
    // - ADPS[2:0] en 1: Establece el factor de división del reloj (Prescaler) a 128.
    //   Cálculo: 16 MHz / 128 = 125 KHz.
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t HAL_ADC_Read(uint8_t channel)
{
    // 1. Filtrar la entrada: Asegurar que el canal pedido no desborde los bits permitidos (0 a 7).
    channel &= 0x07;
    
    // 2. Modificar el multiplexor de entrada (MUX):
    // Se utiliza una máscara lógica AND (0xF8 = 0b11111000) para barrer (poner a 0) 
    // únicamente los 3 bits inferiores correspondientes al canal previo, respetando 
    // la configuración de los bits superiores de referencia (REFS). 
    // Luego, se inyecta el nuevo canal mediante un OR lógico.
    ADMUX = (ADMUX & 0xF8) | channel;
    
    // 3. Iniciar la conversión de hardware estableciendo el bit ADSC (ADC Start Conversion).
    ADCSRA |= (1 << ADSC);
    
    // 4. Polling (Espera Activa):
    // El núcleo del microcontrolador se detiene aquí evaluando el flag ADSC.
    // El hardware borrará automáticamente (pondrá a 0) este bit cuando la conversión finalice
    // (aproximadamente luego de 13 a 25 ciclos de reloj del ADC, unos ~104 microsegundos).
    while (ADCSRA & (1 << ADSC));
    
    // 5. Lectura de resultados:
    // Retornamos el pseudo-registro de 16 bits `ADC`. 
    // El compilador AVR-GCC automáticamente lo traduce en dos instrucciones de ensamblador 
    // ordenadas que garantizan la lectura de `ADCL` primero y `ADCH` después, 
    // como exige el hardware para evitar el bloqueo del bus de datos del registro.
    return ADC;
}