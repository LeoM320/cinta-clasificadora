/**
 * @file debounce.h
 * @brief Filtro temporal anti-rebote (Debounce) genérico para señales digitales.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 *
 * @details
 * Este módulo proporciona una máquina de estados para eliminar el ruido electromecánico 
 * (rebotes o *bouncing*) que se produce al cerrar o abrir contactos mecánicos como 
 * pulsadores, relés o finales de carrera. 
 * Su diseño está desacoplado del hardware: no lee pines directamente, sino que 
 * procesa una señal booleana abstracta inyectada a través de su función de actualización.
 * Utiliza el módulo de temporizadores no bloqueantes para evaluar la estabilidad de la señal.
 */

#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

#include <stdint.h>
#include <stdbool.h>
#include "temporizador.h"

/**
 * @brief Estructura de contexto para la máquina de estados del filtro anti-rebote.
 * 
 * @note Se debe instanciar una estructura independiente por cada señal física 
 *       que se desee filtrar.
 */
typedef struct {
    bool estado_validado;   /**< El estado lógico limpio, estable y filtrado (salida) */
    bool estado_previo;     /**< Memoria de la última lectura cruda para detectar transiciones (flancos) */
    Temporizador timer;     /**< Temporizador para medir el tiempo de estabilidad de la señal */
    uint32_t tiempo_ms;     /**< Tiempo mínimo en milisegundos que la señal debe permanecer estable para ser validada */
} Debouncer_t;

/**
 * @brief Inicializa la estructura del filtro anti-rebote.
 * 
 * @param[in,out] d              Puntero a la instancia del filtro.
 * @param[in]     tiempo_ms      Tiempo de guarda (debounce time) en milisegundos.
 *                               Valores típicos rondan entre 10ms y 50ms según el interruptor.
 * @param[in]     estado_inicial Estado lógico inicial esperado (ej. `true` si usa resistencia Pull-Up).
 */
void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial);

/**
 * @brief Evalúa una nueva lectura cruda y actualiza el estado del filtro.
 * 
 * @note Esta función debe ser llamada periódicamente (ej. en el Super Loop o 
 *       en una tarea de polling) inyectando la lectura actual del hardware.
 * 
 * @param[in,out] d             Puntero a la instancia del filtro.
 * @param[in]     lectura_cruda El estado lógico inmediato leído desde el hardware.
 * @return true                 Si el estado validado (limpio) es ALTO.
 * @return false                Si el estado validado (limpio) es BAJO.
 */
bool Debounce_Update(Debouncer_t *d, bool lectura_cruda);

#endif /* DEBOUNCE_H_ */