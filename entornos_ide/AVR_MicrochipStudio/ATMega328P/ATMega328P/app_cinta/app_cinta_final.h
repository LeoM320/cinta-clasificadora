#ifndef APP_CINTA_FINAL_H_
#define APP_CINTA_FINAL_H_

// 1. Librerías estándar del compilador
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 2. Dependencias del proyecto (HAL, Drivers y Configuración)
#include "hal/include/hal_timer.h"
#include "hal/include/hal_servo.h"
#include "drivers/hcsr04.h"
#include "utils/debounce.h"
#include "config/gpio.h"

// ============================================================================
// CONFIGURACIÓN Y PARAMETRIZACIÓN FÍSICA (Matemática entera en mm)
// ============================================================================
#define ALTURA_CAJA_CHICA           6   // cm
#define ALTURA_CAJA_MEDIANA         8   // cm
#define ALTURA_CAJA_GRANDE          10  // cm
#define TOLERANCIA_MEDICION_CM      1   // +/- 1 cm de margen

#define LARGO_CAJA_ESTIMADO_MM      100 // Largo promedio de la caja
#define DISTANCIA_SENSOR_SERVO_MM   30  // Distancia física del ultrasónico al primer servo
#define ALTURA_SENSOR_PISO_CM       20  // Altura fija de montaje del HC-SR04

#define MAX_CAJAS_EN_CINTA          20  // Capacidad máxima del Ring Buffer

// ============================================================================
// ESTRUCTURAS DE DATOS Y MÁQUINA DE ESTADOS
// ============================================================================

/**
 * @brief Estados limpios para la gestión de ingreso y telemetría ultrasónica.
 */
typedef enum {
    CINTA_FINAL_OFF,
    CINTA_FINAL_IDLE,
    CINTA_FINAL_ESPERANDO_MEDICION
} eCintaFinalState;

/**
 * @brief Modelo de datos para representar una caja en tránsito.
 */
typedef struct {
    uint8_t altura;          // Altura calculada en cm
    uint8_t destino_salida;  // ID de salida asignada (1, 2 o 3)
    uint32_t tick_eta;       // Marca de tiempo estimada de llegada (Timestamp)
} sCajaFinal;

/**
 * @brief Control asíncrono para el disparo cinemático y retracción de cada servo.
 */
typedef struct {
    bool esperando_activacion;
    uint32_t tick_programado;  // Momento exacto de impacto
    bool en_movimiento;
    uint32_t tick_inicio;      // Momento en que se movió el brazo (para la retracción)
} sServoFinal;

/**
 * @brief Historial lógico de flancos para los sensores ópticos.
 */
typedef struct {
    uint8_t last_state;
    uint8_t actual_state;
} sSensorFinal;

/**
 * @brief FIFO circular para el seguimiento estricto de las cajas por zona.
 */
typedef struct {
    sCajaFinal buffer[MAX_CAJAS_EN_CINTA];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} sColaCajasFinal;

// Puntero de callback para reportar alarmas asíncronas a main -> UART/Qt
typedef void (*CintaFinalErrorCallback_t)(uint8_t codigo_error);

// ============================================================================
// API PÚBLICA DE LA APLICACIÓN
// ============================================================================
void App_CintaFinal_Init(void);
void App_CintaFinal_Task(void);
void App_CintaFinal_SetEstado(bool encender);
void App_CintaFinal_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada);
void App_CintaFinal_SetErrorCallback(CintaFinalErrorCallback_t callback);

#endif /* APP_CINTA_FINAL_H_ */