/**
 * @file hal_uart.c
 * @brief Implementación interna del driver UART y rutinas de interrupción (ISR).
 */

#include "../../include/hal_uart.h"
#include "../../../config/hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/** @brief Tamaño del Buffer Circular RX. Debe ser potencia de 2 para optimizar las operaciones de módulo. */
#define RX_BUFFER_SIZE 128

/* 
 * ==========================================
 * MEMORIA COMPARTIDA (ISR <-> Super Loop)
 * ==========================================
 * Nota de diseño: Se utiliza el cualificador 'volatile' para obligar al compilador 
 * a leer estas variables directamente desde la RAM en cada acceso, previniendo 
 * que las cachee en registros del microcontrolador. Esto es vital porque la ISR 
 * altera sus valores de forma asíncrona (fuera del flujo secuencial del programa).
 */

/** @brief Array de almacenamiento lineal estructurado lógicamente como anillo. */
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];

/** @brief Índice de escritura: Modificado EXCLUSIVAMENTE por la ISR. Indica dónde caerá el próximo byte. */
volatile uint8_t rx_head = 0; 

/** @brief Índice de lectura: Modificado EXCLUSIVAMENTE por el Super Loop (HAL_UART_RxRead). */
volatile uint8_t rx_tail = 0; 

void HAL_UART_Init(uint32_t baudrate)
{
    // Cálculo del divisor (UBRR) usando el modo Double Speed (divisor de 8 en lugar de 16)
    // Esto reduce el error de truncamiento en los baudios para relojes típicos (ej. 16 MHz).
    uint16_t ubrr_val = (F_CPU / 8 / baudrate) - 1;
    
    UBRR0H = (uint8_t)(ubrr_val >> 8);
    UBRR0L = (uint8_t)ubrr_val;
    
    // Encendemos el bit U2X0 (Double Transmission Speed)
    UCSR0A = (1 << U2X0);
    
    // Habilitar transmisor (TX), receptor (RX) y la Interrupción de recepción (RXCIE)
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    
    // Formato de trama: 8 bits de datos, 1 stop bit, sin paridad
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void HAL_UART_TxByte(uint8_t data)
{
    // Polling sobre el Data Register Empty flag (UDRE0).
    // Espera activa hasta que el buffer de hardware esté listo para un nuevo byte.
    while (!(UCSR0A & (1 << UDRE0)));
    
    // Cargar el dato en el registro desencadena inmediatamente la transmisión física.
    UDR0 = data; 
}

void HAL_UART_TxString(const char* str)
{
    // Recorrer el array de memoria hasta el delimitador null.
    while (*str) {
        HAL_UART_TxByte(*str++);
    }
}

bool HAL_UART_RxDataAvailable(void)
{
    // Operación atómica de un solo ciclo en AVR por tratarse de variables de 8 bits.
    // Si los punteros virtuales se desfasaron, el anillo contiene datos útiles.
    return (rx_head != rx_tail);
}

uint8_t HAL_UART_RxRead(void)
{
    // Protección contra lecturas en vacío.
    if (rx_head == rx_tail) {
        return 0;
    }
    
    // Leer el dato apuntado por la cola actual.
    uint8_t data = rx_buffer[rx_tail];
    
    // Avanzar el puntero de lectura de forma circular.
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    
    return data;
}

// ==========================================
// INTERRUPCIÓN DE RECEPCIÓN (Hardware RX)
// ==========================================

/**
 * @brief Rutina de Servicio de Interrupción (ISR) para recepción UART (RX Complete).
 * 
 * @details 
 * Esta ISR se dispara automáticamente por hardware cada vez que ingresa un byte completo.
 * Su objetivo primordial es extraer el dato de `UDR0` lo más rápido posible para prevenir
 * condiciones de Data Overrun, y encolarlo en el Ring Buffer de software.
 */
ISR(USART_RX_vect)
{
    // 1. Leer el byte del hardware inmediatamente para liberar el registro y apagar el flag RXC.
    uint8_t data = UDR0;
    
    // 2. Calcular prospectivamente la próxima posición de la cabeza.
    uint8_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
    
    // 3. Estrategia de evasión de colisión (Data Drop):
    // Solo avanzamos y escribimos si el buffer NO está lleno. 
    // Si `next_head` alcanza a `rx_tail`, significa que el Super Loop no está consumiendo 
    // datos lo suficientemente rápido. En este caso crítico, el byte entrante se 
    // descarta silenciosamente para proteger la integridad temporal de los datos viejos.
    if (next_head != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
}