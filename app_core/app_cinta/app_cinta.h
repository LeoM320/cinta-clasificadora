#ifndef APP_CINTA_H_
#define APP_CINTA_H_

// 1. Librerías estándar
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 2. Configuración de hardware y HAL
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_timer.h"
#include "../hal/include/hal_servo.h"

// ==========================================
// DEFINICIONES Y MACROS
// ==========================================
#define ALTURA_CAJA_CHICA    6
#define ALTURA_CAJA_MEDIANA  8
#define ALTURA_CAJA_GRANDE   10
#define TOLERANCIA_MEDICION  1

#define PIN_ECHO             PB2
#define PIN_TRIGGER          PB1

// Macros físicas faltantes (Ajustá estos valores según tu maqueta)
#define DISTANCIA_SENSOR_SERVO_MM 300 
#define LARGO_CAJA_MM             100 

#define MAX_CAJAS_EN_CINTA 20

#define LEER_SENSOR_S1() (HAL_GPIO_READ(IR1_PIN_REG, IR1_PIN))
#define LEER_SENSOR_S2() (HAL_GPIO_READ(IR2_PIN_REG, IR2_PIN))
#define LEER_SENSOR_S3() (HAL_GPIO_READ(IR3_PIN_REG, IR3_PIN))

// ==========================================
// ESTRUCTURAS DE DATOS Y ESTADOS
// ==========================================

typedef enum {
    CINTA_OFF,
    CINTA_CALIBRANDO,
    CINTA_IDLE,
    CINTA_TRIGGER_ON,
    CINTA_ESPERANDO_ECHO,
    CINTA_MIDIENDO_ECHO,
    CINTA_ESPERANDO_MEDICION, // <- Agregado para solucionar tu error
    CINTA_EN_TRANSITO,
    CINTA_EYECTANDO
} eCintaState;

// Estructura de la Caja (Con alias para sCaja y _sCaja)
typedef struct {
    uint8_t altura;
    uint8_t destino_salida;
    uint32_t tick_eta;
} sCaja;
typedef sCaja _sCaja; // Alias para evitar el error "_sCaja"

// Cola Circular
typedef struct {
    sCaja buffer[MAX_CAJAS_EN_CINTA];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} sColaCajas;
typedef sColaCajas _sColaCajas;

// Estado del Servo (Con los campos faltantes agregados)
typedef struct {
    bool en_movimiento;
    uint32_t tick_inicio;
    bool esperando_activacion; // <- Faltaba
    uint32_t tick_programado;  // <- Faltaba
} sEstadoServo;
typedef sEstadoServo _sEstadoServo;

// Estado de Sensores
typedef struct {
    uint8_t last_state;
    uint8_t actual_state;
} sSensores;
typedef sSensores _sSensores;

// ==========================================
// API PÚBLICA
// ==========================================
void App_Cinta_Init(void);
void App_Cinta_Task(void);
void App_Cinta_ConfigurarSalida(uint8_t salida_idx, uint8_t altura_asignada);

#endif /* APP_CINTA_H_ */