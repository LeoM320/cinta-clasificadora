// ==========================================
// Utils/temporizador.c
// ==========================================

/**
 * @file temporizador.c
 * @brief Implementación de la lógica de los temporizadores de software.
 *
 * @details
 * Este módulo delega la obtención del tiempo del sistema a la capa de abstracción 
 * de hardware (HAL). Aprovecha las propiedades de la aritmética de enteros sin signo (unsigned)
 * en C para calcular diferencias de tiempo de manera segura, garantizando inmunidad 
 * frente al desbordamiento natural (overflow/rollover) de los contadores internos del hardware.
 */

#include "temporizador.h"
#include "../hal/include/hal_timer.h" // Usamos la HAL, no los registros

/**
 * @brief Inicia o reinicia un temporizador con resolución de microsegundos.
 * 
 * @param[in,out] t  Puntero a la instancia del temporizador.
 * @param[in]     us Duración deseada en microsegundos.
 */
void Temp_IniciarUS(Temporizador *t, uint32_t us)
{
    // Ya no hace falta ATOMIC_BLOCK acá, HAL_GetMicros lo maneja de forma segura.
    t->inicio = HAL_GetMicros();
    t->tiempo = us;
    t->activo = true;
    t->unidad = TEMP_US;
}

/**
 * @brief Inicia o reinicia un temporizador con resolución de milisegundos.
 * 
 * @param[in,out] t  Puntero a la instancia del temporizador.
 * @param[in]     ms Duración deseada en milisegundos.
 */
void Temp_IniciarMS(Temporizador *t, uint32_t ms)
{
    t->inicio = HAL_GetMillis();
    t->tiempo = ms;
    t->activo = true;
    t->unidad = TEMP_MS;
}

/**
 * @brief Comprueba si el tiempo programado ha transcurrido.
 * 
 * Si el temporizador expira, su estado interno (`activo`) pasa automáticamente 
 * a `false` para evitar lecturas repetidas de un mismo evento.
 * 
 * @note La condición de expiración evalúa `(ahora - t->inicio) >= t->tiempo`. 
 *       Al tratarse de variables `uint32_t`, el estándar de C asegura que la resta 
 *       absorba el desbordamiento circular del contador sin producir valores negativos 
 *       ni fallos lógicos.
 * 
 * @param[in,out] t Puntero a la instancia del temporizador.
 * @return true     El tiempo ha expirado.
 * @return false    El temporizador sigue en curso o fue detenido previamente.
 */
bool Temp_Expiro(Temporizador *t)
{
    if(!t->activo) return false;
    
    uint32_t ahora = (t->unidad == TEMP_US) ? HAL_GetMicros() : HAL_GetMillis();
    
    // Tu tip de C llevado a la práctica: la resta de unsigned absorbe el overflow.
    if((ahora - t->inicio) >= t->tiempo)
    {
        t->activo = false;
        return true;
    }
    return false;
}

/**
 * @brief Reinicia un temporizador utilizando la misma duración y unidad previamente configuradas.
 * 
 * @param[in,out] t Puntero a la instancia del temporizador a reiniciar.
 */
void Temp_Reiniciar(Temporizador *t)
{
    t->inicio = (t->unidad == TEMP_US) ? HAL_GetMicros() : HAL_GetMillis();
    t->activo = true;
}

/**
 * @brief Detiene la cuenta de un temporizador de manera forzada.
 * 
 * Tras invocar esta función, `Temp_Expiro()` siempre retornará `false` 
 * hasta que el temporizador sea reiniciado.
 * 
 * @param[in,out] t Puntero a la instancia del temporizador a detener.
 */
void Temp_Detener(Temporizador *t)
{
    t->activo = false;
}