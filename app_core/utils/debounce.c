/**
 * @file debounce.c
 * @brief Implementación lógica del filtro anti-rebote y detector de flancos.
 */

#include "debounce.h"

void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial) {
    d->estado_validado = estado_inicial;
    d->estado_previo = estado_inicial;
    
    // Inicializamos los flancos apagados
    d->flanco_subida = false;
    d->flanco_bajada = false;
    
    d->tiempo_ms = tiempo_ms;
    Temp_IniciarMS(&d->timer, tiempo_ms);
}

bool Debounce_Update(Debouncer_t *d, bool lectura_cruda) {
    
    // 1. Limpiamos los flancos del ciclo anterior. 
    // Esto garantiza que el aviso de flanco dure exactamente 1 iteración del Super Loop.
    d->flanco_subida = false;
    d->flanco_bajada = false;

    // 2. Si detectamos inestabilidad respecto a la última lectura cruda
    if (lectura_cruda != d->estado_previo) {
        Temp_Reiniciar(&d->timer); 
        d->estado_previo = lectura_cruda;
    }

    // 3. Si la señal cruda se ha mantenido constante el tiempo exigido
    if (Temp_Expiro(&d->timer)) {
        
        // Verificamos si este nuevo estado estable es diferente a la salida que teníamos
        if (d->estado_validado != d->estado_previo) {
            
            // ¡Acá ocurre la transición validada! Evaluamos la dirección del flanco
            if (d->estado_previo == true) {
                d->flanco_subida = true;  // Pasó de LOW a HIGH
            } else {
                d->flanco_bajada = true;  // Pasó de HIGH a LOW
            }
            
            // Actualizamos la salida con el nuevo estado consolidado
            d->estado_validado = d->estado_previo;
        }
    }

    return d->estado_validado;
}