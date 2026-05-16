/**
 * @file hal_uart.h
 * @brief Controlador UART con recepción asíncrona mediante Ring Buffer.
 * @author LeoM320
 * @date 15/05/2026
 */

#ifndef HAL_UART_H_
#define HAL_UART_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el hardware UART (8 bits de datos, 1 stop bit, sin paridad).
 * @param baudrate Velocidad de comunicación (ej. 9600, 115200).
 */
void HAL_UART_Init(uint32_t baudrate);

/**
 * @brief Transmite un único byte por hardware (Bloqueante por ~86us a 115200 bps).
 * @param data Byte a transmitir.
 */
void HAL_UART_TxByte(uint8_t data);

/**
 * @brief Transmite una cadena de caracteres terminada en null ('\0').
 * @param str Puntero a la cadena de texto.
 */
void HAL_UART_TxString(const char* str);

/**
 * @brief Verifica si hay datos sin leer en el buffer de recepción.
 * @return true si hay al menos un byte disponible.
 */
bool HAL_UART_RxDataAvailable(void);

/**
 * @brief Lee el byte más antiguo del buffer de recepción.
 * @return El byte leído, o 0 si el buffer está vacío.
 */
uint8_t HAL_UART_RxRead(void);

#endif /* HAL_UART_H_ */