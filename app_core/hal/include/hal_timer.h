// ==========================================
// HAL/hal_timer.h
// ==========================================

/**
 * @file hal_timer.h
 * @brief Capa de Abstracción de Hardware (HAL) para el reloj base del sistema (SysTick).
 * @author LeoM320
 * @date 19/05/2026
 *
 * @details
 * Proporciona la base de tiempo unificada para toda la aplicación.
 * Este módulo abstrae el Timer0 del microcontrolador (AVR) para generar un "tick"
 * cada 1 milisegundo, sirviendo como núcleo para el manejo de temporizadores 
 * por software no bloqueantes, sin exponer registros específicos de hardware.
 */

#ifndef HAL_TIMER_H_
#define HAL_TIMER_H_

#include <stdint.h>

/**
 * @brief Inicializa el hardware del Timer0 como reloj base.
 * 
 * Configura el periférico en modo Clear Timer on Compare Match (CTC) para disparar 
 * una interrupción exacta cada 1 milisegundo, asumiendo un reloj principal de 16MHz.
 */
void HAL_Timer0_Init(void);

/**
 * @brief Obtiene el tiempo transcurrido desde el inicio del sistema en milisegundos.
 * 
 * @note La lectura es segura ante interrupciones (thread-safe), por lo que puede 
 *       ser llamada desde cualquier parte del Super Loop sin riesgo de corrupción de datos.
 * 
 * @return uint32_t Milisegundos transcurridos.
 */
uint32_t HAL_GetMillis(void);

/**
 * @brief Obtiene el tiempo transcurrido con alta resolución (microsegundos).
 * 
 * @note Utiliza un método de interpolación combinando el contador de milisegundos por 
 *       software y el registro en tiempo real del hardware para ofrecer resolución 
 *       de microsegundos sin disparar interrupciones adicionales que saturen la CPU.
 * 
 * @return uint32_t Microsegundos transcurridos.
 */
uint32_t HAL_GetMicros(void);

#endif /* HAL_TIMER_H_ */