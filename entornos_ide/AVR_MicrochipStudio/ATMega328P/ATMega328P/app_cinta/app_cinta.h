#ifndef APP_CINTA_H_
#define APP_CINTA_H_

#include "hal/include/hal_timer.h"
#include "hal/include/hal_servo.h"
#include "drivers/hcsr04.h"
#include "utils/debounce.h"
#include "config/gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Definición de umbrales por defecto para clasificación (en centímetros)
#define ALTURA_CAJA_CHICA    6
#define ALTURA_CAJA_MEDIANA  8
#define ALTURA_CAJA_GRANDE   10
#define TOLERANCIA_MEDICION  1 // +/- 1 cm

// Parametrización física de la cinta (en milímetros para matemática entera)
#define LARGO_CAJA_MM               100  // 10 cm de largo estimado
#define DISTANCIA_SENSOR_SERVO_MM   30   // 3 cm de separación en la maqueta

// Capacidad del Ring Buffer para cajas en tránsito
#define MAX_CAJAS_EN_CINTA 20

// Enumeración limpia de los estados de la máquina de ingreso
typedef enum {
    CINTA_OFF,
    CINTA_IDLE,
    CINTA_ESPERANDO_MEDICION
} _eCintaState;

// Estructura de una caja
typedef struct {
    uint8_t altura;
    uint8_t destino_salida; // 1, 2 o 3 correspondientes a S1, S2, S3
    uint32_t tick_eta;
} _sCaja;

// Estructura para el control asíncrono de cada servo de forma independiente
typedef struct {
    bool esperando_activacion; // Bandera para saber si el servo está esperando su momento
    uint32_t tick_programado;  // Momento exacto en el que debe golpear
    bool en_movimiento;
    uint32_t tick_inicio;      // Momento en que empezó a moverse para luego retraerlo
} _sEstadoServo;

// Estructura para guardar los estados de los sensores
typedef struct {
    uint8_t last_state;
    uint8_t actual_state;
} _sSensores;

// FIFO para el seguimiento de cajas
typedef struct {
    _sCaja buffer[MAX_CAJAS_EN_CINTA];
    uint8_t head; // Índice de inserción (PUSH)
    uint8_t tail; // Índice de extracción (POP)
    uint8_t count;
} _sColaCajas;

typedef void (*ErrorCallback_t)(uint8_t codigo_error);

// API Pública
void App_Cinta_Init(void);
void App_Cinta_Task(void);
// Función para recibir la configuración dinámica desde el protocolo (UART/Qt)
void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada);
void App_Cinta_SetEstado(bool encender);

void App_Cinta_SetErrorCallback(ErrorCallback_t callback);

#endif /* APP_CINTA_H_ */