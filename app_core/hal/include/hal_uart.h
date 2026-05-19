/**
 * @file hal_uart.h
 * @brief Capa de Abstracción de Hardware (HAL) para el periférico UART en AVR.
 * @author LeoM320
 * @date 15/05/2026
 *
 * @details
 * Este módulo expone la interfaz para la comunicación serie asíncrona.
 * Emplea un enfoque asimétrico diseñado para microcontroladores AVR (ej. ATmega328P):
 * - **Recepción (RX):** Asíncrona y no bloqueante. Utiliza una interrupción por hardware (ISR)
 *   que captura los bytes entrantes y los encola en un buffer circular (Ring Buffer) 
 *   en memoria RAM. Esto desacopla la lectura física del procesamiento lógico.
 * - **Transmisión (TX):** Síncrona y bloqueante mediante polling de registros.
 */

#ifndef HAL_UART_H_
#define HAL_UART_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el hardware UART y configura sus interrupciones.
 * 
 * Configura la interfaz a 8 bits de datos, 1 bit de parada (stop bit) y sin paridad (8N1).
 * Activa el modo de transmisión a doble velocidad (U2X0) por defecto para minimizar
 * los errores de muestreo a altas tasas de transferencia.
 * 
 * @param[in] baudrate Velocidad de comunicación deseada en baudios (ej. 9600, 115200).
 */
void HAL_UART_Init(uint32_t baudrate);

/**
 * @brief Transmite un único byte directamente por el hardware (Polling).
 * 
 * @warning Esta función es bloqueante. Entra en un bucle de espera (polling) hasta que 
 *          el registro de desplazamiento (UDR0) esté libre. A 115200 bps, esto detiene 
 *          el Super Loop por aproximadamente ~86 microsegundos por byte.
 * 
 * @param[in] data Byte crudo a transmitir.
 */
void HAL_UART_TxByte(uint8_t data);

/**
 * @brief Transmite una cadena de caracteres alfanuméricos por el puerto serie.
 * 
 * Invoca internamente a `HAL_UART_TxByte` de forma iterativa.
 * 
 * @param[in] str Puntero a la cadena de texto, la cual debe estar estrictamente 
 *                terminada en el carácter nulo (`\0`).
 */
void HAL_UART_TxString(const char* str);

/**
 * @brief Consulta el estado de la cola del buffer de recepción.
 * 
 * @return true  Si hay al menos un byte pendiente de lectura en el buffer.
 * @return false Si el buffer circular está vacío.
 */
bool HAL_UART_RxDataAvailable(void);

/**
 * @brief Extrae el byte más antiguo almacenado en el buffer circular de recepción (FIFO).
 * 
 * @note Si la función se llama cuando el buffer está vacío, retornará 0. 
 *       Se recomienda envolver esta llamada en un condicional evaluando previamente 
 *       `HAL_UART_RxDataAvailable()`.
 * 
 * @return uint8_t El byte leído.
 */
uint8_t HAL_UART_RxRead(void);

#endif /* HAL_UART_H_ */