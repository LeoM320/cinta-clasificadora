// ==========================================
// Utils/temporizador.h
// ==========================================

/**
 * @file temporizador.h
 * @brief Interfaz para el manejo de temporizadores de software no bloqueantes.
 *
 * @details
 * Proporciona estructuras y prototipos para implementar retardos asíncronos y 
 * control de tiempos sin detener el flujo de ejecución principal del microcontrolador. 
 * Es la alternativa eficiente a los retardos bloqueantes (delays), permitiendo 
 * la ejecución concurrente de lazos de control, lectura de sensores y máquinas de estado.
 */

#ifndef TEMPORIZADORES_H_
#define TEMPORIZADORES_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enumeración de las bases de tiempo soportadas.
 */
typedef enum {
    TEMP_US,    /**< Base de tiempo en microsegundos (us) */
    TEMP_MS     /**< Base de tiempo en milisegundos (ms) */
} TempUnidad;

/**
 * @brief Estructura de control para un temporizador de software.
 * @note Se recomienda inicializarla siempre mediante las funciones provistas 
 *       en lugar de modificar sus campos manualmente.
 */
typedef struct {
    uint32_t inicio;      /**< Marca de tiempo al momento de iniciar el temporizador */
    uint32_t tiempo;      /**< Duración objetivo del temporizador */
    bool activo;          /**< Estado actual: true si está contando, false si expiró o se detuvo */
    TempUnidad unidad;    /**< Resolución configurada (milisegundos o microsegundos) */
} Temporizador;

void Temp_IniciarUS(Temporizador *t, uint32_t us);
void Temp_IniciarMS(Temporizador *t, uint32_t ms);
bool Temp_Expiro(Temporizador *t);
void Temp_Reiniciar(Temporizador *t);
void Temp_Detener(Temporizador *t);

#endif /* TEMPORIZADORES_H_ */