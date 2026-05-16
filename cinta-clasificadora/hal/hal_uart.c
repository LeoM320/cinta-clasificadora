/**
 * @file hal_uart.c
 * @brief Implementación del driver UART con interrupciones para RX.
 */

#include "hal_uart.h"
#include "../config/hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Definición de F_CPU por si no está en las banderas del compilador
#ifndef F_CPU
#define F_CPU 16000000UL 
#endif

// Tamaño del Buffer Circular (Debe ser potencia de 2 preferentemente)
#define RX_BUFFER_SIZE 64

// Memoria compartida entre la ISR y el Super Loop
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0; // Donde escribe la ISR
volatile uint8_t rx_tail = 0; // Donde lee el Super Loop

void HAL_UART_Init(uint32_t baudrate)
{
    // Usamos divisor de 8 en lugar de 16 para el modo Double Speed
    uint16_t ubrr_val = (F_CPU / 8 / baudrate) - 1;
    
    UBRR0H = (uint8_t)(ubrr_val >> 8);
    UBRR0L = (uint8_t)ubrr_val;
    
    // Encendemos el bit U2X0 (Double Transmission Speed)
    UCSR0A = (1 << U2X0);
    
    // Habilitar TX, RX y la Interrupción RX
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    
    // 8 bits de datos, 1 stop bit, sin paridad
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void HAL_UART_TxByte(uint8_t data)
{
    // Esperar hasta que el registro de transmisión esté vacío (UDRE0 = 1)
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data; // Cargar el dato dispara la transmisión física
}

void HAL_UART_TxString(const char* str)
{
    // Recorrer el string hasta encontrar el caracter nulo de fin
    while (*str) {
        HAL_UART_TxByte(*str++);
    }
}

bool HAL_UART_RxDataAvailable(void)
{
    // Si la cabeza y la cola son distintas, hay datos nuevos
    return (rx_head != rx_tail);
}

uint8_t HAL_UART_RxRead(void)
{
    if (rx_head == rx_tail) {
        return 0; // Buffer vacío
    }
    
    // Leer el dato de la cola
    uint8_t data = rx_buffer[rx_tail];
    
    // Avanzar la cola de forma circular
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    
    return data;
}

// ==========================================
// INTERRUPCIÓN DE RECEPCIÓN (Hardware RX)
// ==========================================
ISR(USART_RX_vect)
{
    // 1. Leer el byte del hardware inmediatamente para liberar el registro
    uint8_t data = UDR0;
    
    // 2. Calcular la próxima posición de la cabeza
    uint8_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
    
    // 3. Guardar el dato solo si el buffer no está lleno
    if (next_head != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
    // Nota: Si el buffer se llena, el byte se descarta silenciosamente.
}