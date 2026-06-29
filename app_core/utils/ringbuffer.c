#include "ringbuffer.h"
#include <string.h> // Necesario para memcpy

void RingBuffer_Init(RingBuffer_t* rb, void* mem_block, size_t element_size, uint16_t capacity) {
    rb->buffer = (uint8_t*)mem_block;
    rb->element_size = element_size;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool RingBuffer_Push(RingBuffer_t* rb, const void* item) {
    if (rb == NULL || rb->count >= rb->capacity) {
        return false;
    }
    
    // Calculamos el offset de memoria exacto para escribir
    size_t offset = rb->head * rb->element_size;
    
    // Copiamos los bytes del item al buffer
    memcpy(&(rb->buffer[offset]), item, rb->element_size);
    
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;
    
    return true;
}

bool RingBuffer_Pop(RingBuffer_t* rb, void* item) {
    if (rb == NULL || rb->count == 0) {
        return false;
    }
    
    // Calculamos el offset de memoria del elemento a leer
    size_t offset = rb->tail * rb->element_size;
    
    // Si pasaron un puntero de destino, le copiamos el dato
    if (item != NULL) {
        memcpy(item, &(rb->buffer[offset]), rb->element_size);
    }
    
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    
    return true;
}

void* RingBuffer_Peek(RingBuffer_t* rb) {
    if (rb == NULL || rb->count == 0) {
        return NULL;
    }
    
    size_t offset = rb->tail * rb->element_size;
    return &(rb->buffer[offset]);
}

uint16_t RingBuffer_GetCount(RingBuffer_t* rb) {
    return (rb != NULL) ? rb->count : 0;
}