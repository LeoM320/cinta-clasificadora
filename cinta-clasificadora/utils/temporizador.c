// ==========================================
// Utils/temporizador.c
// ==========================================
#include "temporizador.h"
#include "../hal/include/hal_timer.h" // Usamos la HAL, no los registros

void Temp_IniciarUS(Temporizador *t, uint32_t us)
{
    // Ya no hace falta ATOMIC_BLOCK acá, HAL_GetMicros lo maneja.
    t->inicio = HAL_GetMicros();
    t->tiempo = us;
    t->activo = true;
    t->unidad = TEMP_US;
}

void Temp_IniciarMS(Temporizador *t, uint32_t ms)
{
    t->inicio = HAL_GetMillis();
    t->tiempo = ms;
    t->activo = true;
    t->unidad = TEMP_MS;
}

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

void Temp_Reiniciar(Temporizador *t)
{
    t->inicio = (t->unidad == TEMP_US) ? HAL_GetMicros() : HAL_GetMillis();
    t->activo = true;
}

void Temp_Detener(Temporizador *t)
{
    t->activo = false;
}