/**
 * @file command_handler.h
 * @brief Sistema de manejo de comandos UNER para el clasificador.
 * @author LeoM320
 * @date 18 de Mayo de 2026
 */

#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include <stdint.h>
#include "../utils/uner_protocol.h"

/* Comandos de Sistema y Handshake */
#define CMD_ALIVE               0xAB
#define CMD_ALIVE_ACK           0xAC
#define CMD_GOODBYE             0xAD

/* Comandos de Servomotores */
#define CMD_GET_SERVO           0x10
#define CMD_GET_SERVO_RES       0x11
#define CMD_SET_SERVO           0x40
#define CMD_SET_SERVO_RES       0x41

/* Comandos del Sensor Ultrasónico */
#define CMD_GET_ULTRASONIC      0x20
#define CMD_GET_ULTRASONIC_RES  0x21

/* Comandos de la Cinta Transportadora */
#define CMD_GET_CONVEYOR        0x30
#define CMD_GET_CONVEYOR_RES    0x31
#define CMD_CONVEYOR_ON         0x50
#define CMD_CONVEYOR_OFF        0x51
#define CMD_CONVEYOR_CTRL_RES   0x52

/* Comandos de Configuración y Diagnóstico */
#define CMD_HEARTBEAT_CFG       0x60
#define CMD_HEARTBEAT_CFG_RES   0x61
#define CMD_REPORT_ALL          0x70
#define CMD_REPORT_ALL_RES      0x71

void CMD_Init(void);
void CMD_Process(void);

#endif /* COMMAND_HANDLER_H_ */