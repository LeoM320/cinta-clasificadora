/**
 * @file hal_adc.h
 * @brief Capa de Abstracción de Hardware (HAL) para el Conversor Analógico-Digital (ADC).
 * @author LeoM320
 *
 * @details
 * Proporciona la interfaz para la adquisición de señales analógicas utilizando 
 * el hardware ADC interno de 10 bits. 
 * Esta implementación emplea un diseño basado en "polling" (espera activa), 
 * lo que significa que el hilo de ejecución se bloqueará brevemente mientras 
 * el hardware realiza la conversión.
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

#include <stdint.h>

/**
 * @brief Inicializa el periférico ADC y configura sus parámetros de temporización.
 * 
 * @note La referencia de tensión superior se configura a AVCC (típicamente 5V). 
 *       Es imperativo que el pin AVCC del microcontrolador esté correctamente 
 *       desacoplado físicamente con un capacitor externo.
 */
void HAL_ADC_Init(void);

/**
 * @brief Dispara y lee el valor analógico de un canal de forma bloqueante.
 * 
 * @param[in] channel Canal multiplexado a leer (rango válido de 0 a 7, mapeados a los pines A0-A7).
 * @return uint16_t   Resultado de la conversión analógica-digital.
 *                    Rango de 0 (0V) a 1023 (AVCC).
 */
uint16_t HAL_ADC_Read(uint8_t channel);

#endif /* HAL_ADC_H_ */