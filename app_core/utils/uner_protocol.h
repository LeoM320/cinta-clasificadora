/**
 * @file uner_protocol.h
 * @brief Definiciones, estructuras y prototipos del motor de protocolo UNER.
 *
 * @details
 * Este archivo expone la interfaz para un protocolo de comunicación serie 
 * optimizado para sistemas con restricciones de memoria. 
 * Se emplea un enfoque asimétrico: 
 * - **TX (Transmisión):** Utiliza un buffer circular clásico para encolar datos de forma no bloqueante.
 * - **RX (Recepción):** Implementa un esquema "Zero-Copy" basado en una Máquina de Estados Finitos (FSM). 
 *   No utiliza un buffer circular de recepción; en su lugar, procesa cada byte directamente 
 *   desde el periférico (UART) hacia el buffer final del payload "al vuelo".
 */

#ifndef UNER_PROTOCOL_H_
#define UNER_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Tamaño máximo del buffer de transmisión y payload (debe ser potencia de 2 si se usan máscaras bitwise). */
#define UNER_BUF_SIZE 128
/** @brief Máscara para el manejo eficiente (módulo) de los índices del buffer circular. */
#define UNER_BUFLIMIT (UNER_BUF_SIZE - 1)
/** @brief Tiempo máximo en milisegundos para considerar una trama incompleta y reiniciar la FSM. */
#define UNER_REFRESH_MS 70

/**
 * @brief Estados de la máquina finita (FSM) para la recepción (Zero-Copy).
 */
typedef enum {
    UNER_STATE_U = 0,       /**< Esperando el encabezado 'U' */
    UNER_STATE_N,           /**< Esperando el encabezado 'N' */
    UNER_STATE_E,           /**< Esperando el encabezado 'E' */
    UNER_STATE_R,           /**< Esperando el encabezado 'R' */
    UNER_STATE_LENGTH,      /**< Esperando el byte de longitud (length) */
    UNER_STATE_TOKEN,       /**< Esperando el separador ':' */
    UNER_STATE_PAYLOAD,     /**< Recibiendo los bytes de datos */
    UNER_STATE_CHECKSUM     /**< Esperando el byte de verificación XOR */
} UnerState_t;

/**
 * @brief Estructura del buffer circular de transmisión.
 */
typedef struct {
    uint8_t iR;                       /**< Índice de lectura (Read) */
    uint8_t iW;                       /**< Índice de escritura (Write) */
    uint8_t buf[UNER_BUF_SIZE];       /**< Buffer circular para los datos a enviar */
    uint8_t checksum;                 /**< Acumulador de paridad XOR calculada en tiempo real */
} UnerTx_t;

/**
 * @brief Estructura del decodificador "al vuelo" para recepción.
 */
typedef struct {
    uint8_t checksum;                 /**< Acumulador XOR para validar la trama entrante */
    uint8_t length;                   /**< Longitud total esperada de la carga útil */
    uint8_t payload[UNER_BUF_SIZE];   /**< Buffer lineal directo para los datos útiles recibidos */
    uint8_t payloadCount;             /**< Contador de bytes recibidos en el estado PAYLOAD */
    UnerState_t state;                /**< Estado actual de la máquina de decodificación */
} UnerRx_t;

/**
 * @brief Contexto principal del protocolo UNER.
 * @details Agrupa las estructuras de TX y RX, junto con variables de control de flujo temporal.
 */
typedef struct {
    UnerTx_t tx;              /**< Contexto de transmisión */
    UnerRx_t rx;              /**< Contexto de recepción */
    uint32_t reset_time;      /**< Marca de tiempo del último byte procesado (para timeout) */
    bool comando_listo;       /**< Flag que indica la disponibilidad de una trama válida */
} UnerProtocol_t;

// ==========================================
// Inicialización y Máquina de Estados
// ==========================================
void Uner_Init(UnerProtocol_t *u);
void Uner_Recibir(UnerProtocol_t *u, uint32_t current_ms);
void Uner_Transmitir(UnerProtocol_t *u);

// ==========================================
// Transmisión (TX)
// ==========================================
void Uner_AbrirCarga(UnerProtocol_t *u, uint8_t length);
void Uner_Agregar8(UnerProtocol_t *u, uint8_t valor);
void Uner_Agregar16(UnerProtocol_t *u, uint16_t valor);
void Uner_Agregar32(UnerProtocol_t *u, uint32_t valor);
void Uner_CerrarCarga(UnerProtocol_t *u);

// ==========================================
// Recepción (RX)
// ==========================================
bool Uner_Comando(UnerProtocol_t *u);
uint8_t Uner_IDComando(UnerProtocol_t *u);
uint8_t Uner_Obtener8(UnerProtocol_t *u, uint8_t pos);
uint16_t Uner_Obtener16(UnerProtocol_t *u, uint8_t pos);
uint32_t Uner_Obtener32(UnerProtocol_t *u, uint8_t pos);

#endif /* UNER_PROTOCOL_H_ */