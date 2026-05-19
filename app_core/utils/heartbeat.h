/**
 * @file heartbeat.h
 * @brief Secuenciador de destellos (Heartbeat) basado en patrones de 8 bits.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */

#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <stdint.h>

/**
 * @brief Inicializa el sistema de secuencias visuales.
 * @param bit_duration_ms Tiempo en milisegundos que dura cada bit de la secuencia en el hardware.
 * @param initial_sequence Secuencia de 8 bits inicial (ej. 0b10100000 para doble parpadeo).
 */
void Heartbeat_Init(uint32_t bit_duration_ms, uint8_t initial_sequence);

/**
 * @brief Actualiza la secuencia de destellos en tiempo de ejecución.
 * @param new_sequence Nuevo patrón de 8 bits a mostrar.
 * @note El cambio se aplicará de forma continua, el ciclo actual no se interrumpe abruptamente.
 */
void Heartbeat_SetSequence(uint8_t new_sequence);

/**
 * @brief Tarea periódica que procesa y expone el bit actual de la secuencia.
 * @note Debe llamarse continuamente dentro del Super Loop.
 */
void Heartbeat_Task(void);

#endif /* HEARTBEAT_H_ */