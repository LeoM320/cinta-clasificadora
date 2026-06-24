/**
 * @file debounce.h
 * @brief Filtro temporal anti-rebote (Debounce) genérico para señales digitales.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 *
 * @details
 * Este módulo proporciona una máquina de estados para eliminar el ruido electromecánico 
 * (rebotes o *bouncing*) que se produce al cerrar o abrir contactos mecánicos.
 * Además, incorpora detectores de flanco (Edge Detectors) que emiten un pulso 
 * de un solo ciclo cuando la señal se estabiliza en un nuevo estado.
 */

#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

#include <stdint.h>
#include <stdbool.h>
#include "temporizador.h"

/**
 * @brief Estructura de contexto para la máquina de estados del filtro anti-rebote.
 */
typedef struct {
    bool estado_validado;   /**< El estado lógico limpio, estable y filtrado (salida) */
    bool estado_previo;     /**< Memoria de la última lectura cruda para detectar transiciones */
    
    // --- NUEVAS VARIABLES PARA DETECCIÓN DE FLANCOS ---
    bool flanco_subida;     /**< TRUE por un solo ciclo cuando la señal pasa de 0 a 1 y se estabiliza */
    bool flanco_bajada;     /**< TRUE por un solo ciclo cuando la señal pasa de 1 a 0 y se estabiliza */
    
    Temporizador timer;     /**< Temporizador para medir el tiempo de estabilidad de la señal */
    uint32_t tiempo_ms;     /**< Tiempo mínimo en milisegundos de estabilidad exigida */
} Debouncer_t;

/**
 * @brief Inicializa la estructura del filtro anti-rebote.
 */
void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial);

/**
 * @brief Evalúa una nueva lectura cruda, actualiza el estado y calcula los flancos.
 * * @param[in,out] d             Puntero a la instancia del filtro.
 * @param[in]     lectura_cruda El estado lógico inmediato leído desde el hardware.
 * @return bool                 El estado validado (limpio) actual.
 */
bool Debounce_Update(Debouncer_t *d, bool lectura_cruda);

#endif /* DEBOUNCE_H_ */