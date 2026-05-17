#ifndef APP_CINTA_H_
#define APP_CINTA_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/common.h>

// Definición de umbrales para clasificación (en centímetros)
#define ALTURA_CAJA_CHICA    6
#define ALTURA_CAJA_MEDIANA  8
#define ALTURA_CAJA_GRANDE   10
#define TOLERANCIA_MEDICION  1 // +/- 1 cm
#define PIN_ECHO             PB2
#define PIN_TRIGGER          PB1

// Capacidad del Ring Buffer para cajas en tránsito
#define MAX_CAJAS_EN_CINTA 20

// Macros
#define LEER_SENSOR_S1() (HAL_GPIO_READ(PIND, 3))
#define LEER_SENSOR_S2() (HAL_GPIO_READ(PIND, 4))
#define LEER_SENSOR_S3() (HAL_GPIO_READ(PIND, 5))

// Enumeracion de los estado de la máquina
typedef enum {
    CINTA_OFF,
    CINTA_IDLE,
    CINTA_TRIGGER_ON,
    CINTA_ESPERANDO_ECHO,
    CINTA_MIDIENDO_ECHO,
    CINTA_EN_TRANSITO,
    CINTA_EYECTANDO
} eCintaState;

// Estructura de una caja
typedef struct {
    uint8_t altura;
    uint8_t destino_salida; // 1, 2 o 3 correspondientes a S1, S2, S3
} sCaja;

// Estructura para el control asíncrono de cada servo de forma independiente
typedef struct {
    bool en_movimiento;
    uint32_t tick_inicio;
} sEstadoServo;

// Estructura para guardar los estados de los sensores
typedef struct {
    uint8_t last_state;
    uint8_t actual_state;
} sSensores;

// FIFO para el seguimiento de cajas
typedef struct {
    sCaja buffer[MAX_CAJAS_EN_CINTA];
    uint8_t head; // Índice de inserción (PUSH)
    uint8_t tail; // Índice de extracción (POP)
    uint8_t count;
} sColaCajas;

// API Pública
void App_Cinta_Init(void);
void App_Cinta_Task(void);
// Función para recibir la configuración desde la capa del protocolo (UART/Qt)
void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada);

#endif /* APP_CINTA_H_ */