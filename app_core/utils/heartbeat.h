/**
 * @file heartbeat.h
 * @brief Secuenciador de destellos (Heartbeat) basado en patrones de 8 bits.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 *
 * @details
 * Este módulo proporciona un mecanismo de retroalimentación visual asíncrono y 
 * de bajo consumo computacional. Utiliza una máscara de 8 bits donde cada bit 
 * representa el estado físico de un LED (1 = encendido, 0 = apagado) durante un 
 * intervalo de tiempo definido.
 * Es ideal para reportar códigos de error o estados de la máquina de forma visual
 * (ej. `0b10100000` para un doble parpadeo corto seguido de una pausa larga).
 */

#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <stdint.h>

/**
 * @brief Inicializa el sistema de secuencias visuales y configura el hardware.
 * 
 * @param[in] bit_duration_ms Tiempo en milisegundos que dura cada bit de la secuencia.
 *                            Define la "velocidad" de reproducción del patrón.
 * @param[in] initial_sequence Secuencia de 8 bits inicial a reproducir.
 */
void Heartbeat_Init(uint32_t bit_duration_ms, uint8_t initial_sequence);

/**
 * @brief Actualiza la secuencia de destellos en tiempo de ejecución.
 * 
 * @note El cambio se aplicará de forma continua. La función no reinicia el índice actual 
 *       de la secuencia (`bit_index`), por lo que la transición entre estados es suave 
 *       y no bloquea el flujo del programa.
 * 
 * @param[in] new_sequence Nuevo patrón de 8 bits a mostrar.
 */
void Heartbeat_SetSequence(uint8_t new_sequence);

/**
 * @brief Tarea periódica que procesa y expone el bit actual de la secuencia.
 * 
 * @warning Esta función es no bloqueante y depende del temporizador de software. 
 *          Debe llamarse continuamente dentro del bucle principal (Super Loop) 
 *          o en el despachador de tareas para garantizar la temporización correcta.
 */
void Heartbeat_Task(void);

#endif /* HEARTBEAT_H_ */