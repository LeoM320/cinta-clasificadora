/**
 * @file command_handler.c
 * @brief Implementación de la lógica de procesamiento de comandos UNER.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */

#include "command_handler.h"
#include "../config/hardware.h"
#include "../hal/include/hal_gpio.h"
#include "../hal/include/hal_servo.h"
#include <stddef.h>

/* Contexto global del protocolo UNER, definido en main.c */
extern UnerProtocol_t g_uner;

/**
 * @brief Flag que indica si el handshake con la HMI está completo.
 */
static bool g_handshake_completo = false;

/**
 * @brief Ángulos actuales de cada servo para consultas GET.
 */
static uint8_t g_servo_angles[3] = {90, 90, 90};

/* ---------- HANDLERS ---------- */

static void CMD_HandleAlive(void) {
    g_handshake_completo = true;

    Uner_AbrirCarga(&g_uner, 1);
    Uner_Agregar8(&g_uner, CMD_ALIVE_ACK);
    Uner_CerrarCarga(&g_uner);
}

static void CMD_HandleGoodbye(void) {
    g_handshake_completo = false;

    /* Deshabilitar todos los servos por seguridad */
    HAL_Servo_Disable(SERVO_1);
    HAL_Servo_Disable(SERVO_2);
    HAL_Servo_Disable(SERVO_3);

    /* Apagar cinta */
    HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);
}

static void CMD_HandleConveyorOn(void) {
    HAL_GPIO_WRITE_HIGH(CINTA_PORT, CINTA_PIN);

    Uner_AbrirCarga(&g_uner, 2);
    Uner_Agregar8(&g_uner, CMD_CONVEYOR_CTRL_RES);
    Uner_Agregar8(&g_uner, 0x01);
    Uner_CerrarCarga(&g_uner);
}

static void CMD_HandleConveyorOff(void) {
    HAL_GPIO_WRITE_LOW(CINTA_PORT, CINTA_PIN);

    Uner_AbrirCarga(&g_uner, 2);
    Uner_Agregar8(&g_uner, CMD_CONVEYOR_CTRL_RES);
    Uner_Agregar8(&g_uner, 0x01);
    Uner_CerrarCarga(&g_uner);
}

/**
 * @brief Procesa CMD_SET_SERVO: establece el ángulo de un servo específico.
 *
 * @details Payload esperado: [CMD_ID][servo_id][angle]
 * - servo_id: 0 = SERVO_1, 1 = SERVO_2, 2 = SERVO_3
 * - angle: 0 a 180 grados
 */
static void CMD_HandleSetServo(void) {
    uint8_t servo_id = Uner_Obtener8(&g_uner, 1);
    uint8_t angle    = Uner_Obtener8(&g_uner, 2);

    /* Validar rango */
    if (servo_id > 2 || angle > 180) {
        /* Responder con error */
        Uner_AbrirCarga(&g_uner, 3);
        Uner_Agregar8(&g_uner, CMD_SET_SERVO_RES);
        Uner_Agregar8(&g_uner, servo_id);
        Uner_Agregar8(&g_uner, 0x00);  /* 0 = fallo */
        Uner_CerrarCarga(&g_uner);
        return;
    }

    /* Guardar ángulo y mover servo */
    g_servo_angles[servo_id] = angle;
    HAL_Servo_Enable(servo_id);
    HAL_Servo_SetAngle(servo_id, angle);

    /* Responder con éxito */
    Uner_AbrirCarga(&g_uner, 3);
    Uner_Agregar8(&g_uner, CMD_SET_SERVO_RES);
    Uner_Agregar8(&g_uner, servo_id);
    Uner_Agregar8(&g_uner, 0x01);  /* 1 = éxito */
    Uner_CerrarCarga(&g_uner);
}

/**
 * @brief Procesa CMD_GET_SERVO: consulta el ángulo actual de un servo.
 *
 * @details Payload esperado: [CMD_ID][servo_id]
 * Responde con: [CMD_GET_SERVO_RES][servo_id][angle]
 */
static void CMD_HandleGetServo(void) {
    uint8_t servo_id = Uner_Obtener8(&g_uner, 1);

    if (servo_id > 2) {
        return;  /* ID inválido, no responder */
    }

    Uner_AbrirCarga(&g_uner, 3);
    Uner_Agregar8(&g_uner, CMD_GET_SERVO_RES);
    Uner_Agregar8(&g_uner, servo_id);
    Uner_Agregar8(&g_uner, g_servo_angles[servo_id]);
    Uner_CerrarCarga(&g_uner);
}

/* ---------- TABLA DE DESPACHO ---------- */

typedef void (*CmdHandler_t)(void);

static const CmdHandler_t cmd_table[] = {
    [CMD_ALIVE]         = CMD_HandleAlive,
    [CMD_GOODBYE]       = CMD_HandleGoodbye,
    [CMD_GET_SERVO]     = CMD_HandleGetServo,
    [CMD_GET_ULTRASONIC]= NULL,
    [CMD_GET_CONVEYOR]  = NULL,
    [CMD_SET_SERVO]     = CMD_HandleSetServo,
    [CMD_CONVEYOR_ON]   = CMD_HandleConveyorOn,
    [CMD_CONVEYOR_OFF]  = CMD_HandleConveyorOff,
    [CMD_HEARTBEAT_CFG] = NULL,
    [CMD_REPORT_ALL]    = NULL,
};

#define CMD_TABLE_MAX_ID 0xAD

/* ---------- INTERFAZ PÚBLICA ---------- */

void CMD_Init(void) {
    g_handshake_completo = false;
}

void CMD_Process(void) {
    if (!Uner_Comando(&g_uner)) {
        return;
    }

    uint8_t cmd_id = Uner_IDComando(&g_uner);

    /* Handshake y desconexión siempre disponibles */
    if (cmd_id == CMD_ALIVE) {
        CMD_HandleAlive();
        return;
    }

    if (cmd_id == CMD_GOODBYE) {
        CMD_HandleGoodbye();
        return;
    }

    /* Bloquear comandos operativos sin handshake */
    if (!g_handshake_completo) {
        return;
    }

    if (cmd_id <= CMD_TABLE_MAX_ID && cmd_table[cmd_id] != NULL) {
        cmd_table[cmd_id]();
    }
}