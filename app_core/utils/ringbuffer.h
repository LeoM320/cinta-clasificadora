/**
 * @file ringbuffer.h
 * @brief Librería agnóstica para el manejo de Buffers Circulares (FIFO) estáticos.
 * * Implementación diseñada para sistemas embebidos bare-metal. No utiliza asignación
 * dinámica de memoria (malloc). El usuario debe proveer el bloque de memoria
 * durante la inicialización.
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Estructura de control del Ring Buffer.
 * Mantiene el estado y la metadata del buffer.
 */
typedef struct {
    uint8_t* buffer;       /**< Puntero al bloque de memoria estática inyectado */
    size_t element_size;   /**< Tamaño en bytes de cada elemento (ej. sizeof(mi_struct)) */
    uint16_t capacity;     /**< Capacidad máxima (cantidad de elementos) */
    uint16_t head;         /**< Puntero de escritura (Push) */
    uint16_t tail;         /**< Puntero de lectura (Pop) */
    uint16_t count;        /**< Cantidad actual de elementos en la cola */
} RingBuffer_t;

/**
 * @brief Inicializa el Ring Buffer vinculándolo a un espacio de memoria físico.
 * * @param rb Puntero a la estructura de control del Ring Buffer.
 * @param mem_block Puntero al arreglo estático que servirá como almacenamiento.
 * @param element_size Tamaño en bytes de cada elemento a almacenar.
 * @param capacity Cantidad máxima de elementos que puede alojar el bloque.
 */
void RingBuffer_Init(RingBuffer_t* rb, void* mem_block, size_t element_size, uint16_t capacity);

/**
 * @brief Encola un nuevo elemento en el buffer (Copia por valor).
 * * @param rb Puntero a la estructura de control.
 * @param item Puntero al dato que se desea encolar.
 * @return true Si el dato se encoló correctamente.
 * @return false Si el buffer está lleno (Overflow).
 */
bool RingBuffer_Push(RingBuffer_t* rb, const void* item);

/**
 * @brief Desencola el elemento más antiguo del buffer.
 * * @param rb Puntero a la estructura de control.
 * @param item Puntero donde se copiará el dato desencolado. Puede ser NULL si solo se quiere descartar.
 * @return true Si el dato se desencoló correctamente.
 * @return false Si el buffer está vacío (Underflow).
 */
bool RingBuffer_Pop(RingBuffer_t* rb, void* item);

/**
 * @brief Retorna un puntero al próximo elemento a salir sin sacarlo de la cola.
 * * Fundamental para evaluar temporizadores u observar estados sin alterar la FSM.
 * * @param rb Puntero a la estructura de control.
 * @return void* Puntero al elemento más antiguo, o NULL si está vacío.
 */
void* RingBuffer_Peek(RingBuffer_t* rb);

/**
 * @brief Retorna la cantidad de elementos alojados.
 * * @param rb Puntero a la estructura de control.
 * @return uint16_t Cantidad de elementos actuales.
 */
uint16_t RingBuffer_GetCount(RingBuffer_t* rb);

#endif /* RINGBUFFER_H_ */