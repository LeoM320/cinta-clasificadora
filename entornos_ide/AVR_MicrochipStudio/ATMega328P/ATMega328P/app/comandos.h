// ==========================================
// app/comandos.h
// ==========================================

/**
 * @file comandos.h
 * @brief Despachador de comandos (Command Dispatcher) del protocolo UNER.
 * @author LeoM320
 *
 * @details
 * Módulo central de la capa de aplicación. Traduce los paquetes binarios 
 * del protocolo UNER en acciones físicas de hardware (Servos, Cinta) 
 * y recolecta datos de telemetría (Ultrasónico, IR) para enviarlos al Host.
 */

#ifndef APP_COMANDOS_H_
#define APP_COMANDOS_H_

#include <stdint.h>
#include "../utils/uner_protocol.h"

/**
 * @defgroup Command_Dictionary Diccionario de Comandos
 * @brief Identificadores de peticiones (Host -> Dispositivo) y respuestas (Dispositivo -> Host).
 * @note Las respuestas exitosas (ACK) suman 0x80 al ID del comando original.
 * @{
 */

// --- Peticiones del Host (Master) ---
#define CMD_ALIVE           0x01    /**< Petición de estado vital (Devuelve Uptime en ms) */
#define CMD_GET_VERSION     0x02    /**< Petición de versión del firmware */
#define CMD_SET_SERVO       0x03    /**< Mueve un servo. Payload: [Servo_ID(8b)] [Angulo(8b)] */
#define CMD_GET_DISTANCE    0x04    /**< Lee el sensor ultrasónico. Devuelve distancia en cm (16b) */
#define CMD_GET_IR_STATES   0x05    /**< Lee los 4 sensores IR. Devuelve 1 byte empaquetado (Bit-Packing) */
#define CMD_SET_BELT        0x06    /**< Controla la cinta. Payload: [Estado(8b): 0=Apagado, 1=Encendido] */

// --- Respuestas del Dispositivo (Slave/ACK) ---
#define ACK_ALIVE           0x81    
#define ACK_GET_VERSION     0x82    
#define ACK_SET_SERVO       0x83    
#define ACK_GET_DISTANCE    0x84    
#define ACK_GET_IR_STATES   0x85    
#define ACK_SET_BELT        0x86    

// --- Respuestas de Error ---
#define ERR_UNKNOWN_CMD     0xFF    /**< Respuesta a un ID de comando no soportado */
#define ERR_BAD_PAYLOAD     0xFE    /**< Respuesta cuando la longitud de los parámetros es incorrecta */

/** @} */

void Comandos_Procesar(UnerProtocol_t *u);

#endif /* APP_COMANDOS_H_ */