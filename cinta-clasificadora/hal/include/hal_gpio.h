/**
 * @file hal_gpio.h
 * @brief Capa de abstracción de hardware (HAL) minimalista para control de GPIO.
 * @author LeoM320
 * @date 14 de Mayo de 2026
 *
 * @details Este archivo proporciona macros de preprocesador altamente eficientes
 * para la manipulación directa de los registros de entrada/salida del ATMega328P.
 * Al usar macros en lugar de funciones, se evita el tiempo de salto en memoria, 
 * logrando una ejecución en un solo ciclo de reloj de la CPU.
 */

#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_

// ==========================================
// MACROS DE DIRECCIÓN (Entrada / Salida)
// ==========================================

/**
 * @def HAL_GPIO_SET_OUTPUT(ddr, pin)
 * @brief Configura un pin específico del microcontrolador como SALIDA.
 * @param ddr Registro de dirección de datos del puerto (ej. DDRB, DDRD).
 * @param pin Número de pin a configurar (0 a 7).
 */
#define HAL_GPIO_SET_OUTPUT(ddr, pin)  ((ddr) |= (1 << (pin)))

/**
 * @def HAL_GPIO_SET_INPUT(ddr, pin)
 * @brief Configura un pin específico del microcontrolador como ENTRADA.
 * @param ddr Registro de dirección de datos del puerto (ej. DDRB, DDRD).
 * @param pin Número de pin a configurar (0 a 7).
 */
#define HAL_GPIO_SET_INPUT(ddr, pin)   ((ddr) &= ~(1 << (pin)))

// ==========================================
// MACROS DE ESCRITURA (Alto / Bajo)
// ==========================================

/**
 * @def HAL_GPIO_WRITE_HIGH(port, pin)
 * @brief Establece el estado lógico de un pin de salida en ALTO (5V).
 * @param port Registro de datos del puerto (ej. PORTB, PORTD).
 * @param pin Número de pin a modificar (0 a 7).
 */
#define HAL_GPIO_WRITE_HIGH(port, pin) ((port) |= (1 << (pin)))

/**
 * @def HAL_GPIO_WRITE_LOW(port, pin)
 * @brief Establece el estado lógico de un pin de salida en BAJO (0V).
 * @param port Registro de datos del puerto (ej. PORTB, PORTD).
 * @param pin Número de pin a modificar (0 a 7).
 */
#define HAL_GPIO_WRITE_LOW(port, pin)  ((port) &= ~(1 << (pin)))

/**
 * @def HAL_GPIO_TOGGLE(port, pin)
 * @brief Invierte (hace toggle) el estado lógico actual de un pin de salida.
 * @param port Registro de datos del puerto (ej. PORTB, PORTD).
 * @param pin Número de pin a invertir (0 a 7).
 */
#define HAL_GPIO_TOGGLE(port, pin)     ((port) ^= (1 << (pin)))

// ==========================================
// MACROS DE LECTURA
// ==========================================

/**
 * @def HAL_GPIO_READ(pin_reg, pin)
 * @brief Lee el estado lógico físico actual de un pin configurado como entrada.
 * @param pin_reg Registro de pines de entrada del puerto (ej. PINB, PIND).
 * @param pin Número de pin a leer (0 a 7).
 * @return Un valor mayor a 0 si el pin está en estado ALTO, o 0 si está en BAJO.
 */
#define HAL_GPIO_READ(pin_reg, pin)    ((pin_reg) & (1 << (pin)))

#endif /* HAL_GPIO_H_ */