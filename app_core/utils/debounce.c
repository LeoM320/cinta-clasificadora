#include "debounce.h"

void Debounce_Init(Debouncer_t *d, uint32_t tiempo_ms, bool estado_inicial) {
    d->estado_validado = estado_inicial;
    d->estado_previo = estado_inicial;
    d->tiempo_ms = tiempo_ms;
    
    // Iniciamos el temporizador (se reiniciará automáticamente al detectar cambios)
    Temp_IniciarMS(&d->timer, tiempo_ms);
}

bool Debounce_Update(Debouncer_t *d, bool lectura_cruda) {
    // Si hubo una transición en la señal cruda respecto al ciclo anterior
    if (lectura_cruda != d->estado_previo) {
        Temp_Reiniciar(&d->timer); // Reseteamos el reloj de validación
        d->estado_previo = lectura_cruda;
    }

    // Si la señal cruda se mantuvo estable durante el tiempo exigido
    if (Temp_Expiro(&d->timer)) {
        // Y si ese estado estable es diferente al que teníamos validado, lo actualizamos
        if (d->estado_validado != d->estado_previo) {
            d->estado_validado = d->estado_previo;
        }
    }

    return d->estado_validado;
}