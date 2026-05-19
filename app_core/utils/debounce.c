/**
 * @file debounce.c
 * @brief Implementación lógica del filtro anti-rebote.
 *
 * @details
 * El algoritmo implementado funciona como un filtro de integración de tiempo:
 * 1. Cada vez que detecta un cambio (un posible rebote o una pulsación real) entre 
 *    la lectura actual y el estado previo, reinicia el temporizador de validación.
 * 2. Si la señal deja de cambiar (los rebotes mecánicos cesan) y el temporizador 
 *    logra expirar, se considera que la señal se ha estabilizado.
 * 3. Si ese nuevo estado estable difiere del último estado validado, se actualiza la salida.
 */

#include "debounce.h"

/**
 * @brief Prepara el filtro estableciendo los valores iniciales y arrancando el temporizador.
 * 
 * @param[in,out] d              Puntero a la instancia del filtro a inicializar.
 * @param[in]     tiempo_ms      Tiempo de estabilidad exigido para confirmar un cambio.
 * @param[in]     estado_inicial Estado de reposo esperado.
 */
void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial) {
    d->estado_validado = estado_inicial;
    d->estado_previo = estado_inicial;
    d->tiempo_ms = tiempo_ms;
    
    // Iniciamos el temporizador (se reiniciará automáticamente al detectar cambios por el ruido)
    Temp_IniciarMS(&d->timer, tiempo_ms);
}

/**
 * @brief Máquina de estados que filtra el ruido temporal de la señal de entrada.
 * 
 * @param[in,out] d             Puntero a la instancia del filtro.
 * @param[in]     lectura_cruda Muestra actual tomada del hardware.
 * @return bool                 El estado libre de rebotes que puede ser utilizado por la lógica de la aplicación.
 */
bool Debounce_Update(Debouncer_t *d, bool lectura_cruda) {
    
    // Si detectamos un flanco (transición de señal) respecto a la última lectura registrada
    if (lectura_cruda != d->estado_previo) {
        Temp_Reiniciar(&d->timer); // Reseteamos el reloj de validación porque hubo inestabilidad
        d->estado_previo = lectura_cruda;
    }

    // Si la señal cruda se ha mantenido constante durante todo el 'tiempo_ms' exigido
    if (Temp_Expiro(&d->timer)) {
        
        // Verificamos si este nuevo estado estable consolida un cambio real frente al estado de salida actual
        if (d->estado_validado != d->estado_previo) {
            d->estado_validado = d->estado_previo;
        }
    }

    return d->estado_validado;
}