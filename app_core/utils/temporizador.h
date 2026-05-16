// ==========================================
// Utils/temporizador.h
// ==========================================
#ifndef TEMPORIZADORES_H_
#define TEMPORIZADORES_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TEMP_US,
    TEMP_MS
} TempUnidad;

typedef struct {
    uint32_t inicio;
    uint32_t tiempo;
    bool activo;
    TempUnidad unidad;
} Temporizador;

void Temp_IniciarUS(Temporizador *t, uint32_t us);
void Temp_IniciarMS(Temporizador *t, uint32_t ms);
bool Temp_Expiro(Temporizador *t);
void Temp_Reiniciar(Temporizador *t);
void Temp_Detener(Temporizador *t);

#endif /* TEMPORIZADORES_H_ */