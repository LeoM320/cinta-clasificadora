/**
 * @file debounce.h
 * @brief Filtro temporal anti-rebote (Debounce) genérico para señales digitales.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */

#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

#include <stdint.h>
#include <stdbool.h>
#include "temporizador.h"

/**
 * @brief Estructura de contexto para la máquina de estados del filtro.
 */
typedef struct {
    bool estado_validado;   /**< El estado limpio y estable de la señal */
    bool estado_previo;     /**< La última lectura cruda registrada */
    Temporizador timer;     /**< Temporizador para medir la estabilidad */
    uint32_t tiempo_ms;     /**< Tiempo exigido de estabilidad en milisegundos */
} Debouncer_t;

/**
 * @brief Inicializa la estructura del filtro anti-rebote.
 * @param d Puntero a la estructura Debouncer_t.
 * @param tiempo_ms Tiempo que la señal debe permanecer invariable para validarse.
 * @param estado_inicial Estado lógico inicial esperado.
 */
void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial);

/**
 * @brief Actualiza la máquina de estados del filtro con una nueva lectura.
 * @param d Puntero a la estructura Debouncer_t.
 * @param lectura_cruda El estado físico actual leído desde el hardware.
 * @return El estado lógico validado (filtrado).
 */
bool Debounce_Update(Debouncer_t *d, bool lectura_cruda);

#endif /* DEBOUNCE_H_ */