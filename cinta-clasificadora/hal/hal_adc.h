/**
 * @file hal_adc.h
 * @brief Controlador del Conversor Analógico-Digital (ADC).
 * @author LeoM320
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

#include <stdint.h>

/**
 * @brief Inicializa el hardware del ADC con referencia a AVCC (5V).
 */
void HAL_ADC_Init(void);

/**
 * @brief Lee el valor analógico de un pin específico.
 * @param channel Canal a leer (0 a 7, correspondientes a A0-A7).
 * @return Valor convertido (0 a 1023).
 */
uint16_t HAL_ADC_Read(uint8_t channel);

#endif /* HAL_ADC_H_ */