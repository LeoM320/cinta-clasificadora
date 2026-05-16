/**
 * @file uner_protocol.h
 * @brief Motor del protocolo UNER optimizado (Zero-Copy Parsing) en C puro para enteros.
 */

#ifndef UNER_PROTOCOL_H_
#define UNER_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

#define UNER_BUF_SIZE 128
#define UNER_BUFLIMIT (UNER_BUF_SIZE - 1)
#define UNER_REFRESH_MS 70

typedef enum {
    UNER_STATE_U = 0,
    UNER_STATE_N,
    UNER_STATE_E,
    UNER_STATE_R,
    UNER_STATE_LENGTH,
    UNER_STATE_TOKEN,
    UNER_STATE_PAYLOAD,
    UNER_STATE_CHECKSUM
} UnerState_t;

// Buffer circular de transmisión (Armado de paquetes)
typedef struct {
    uint8_t iR;
    uint8_t iW;
    uint8_t buf[UNER_BUF_SIZE];
    uint8_t checksum;
} UnerTx_t;

// Decodificador "al vuelo" (Sin buffer circular)
typedef struct {
    uint8_t checksum;
    uint8_t length;
    uint8_t payload[UNER_BUF_SIZE];
    uint8_t payloadCount;
    UnerState_t state;
} UnerRx_t;

typedef struct {
    UnerTx_t tx;
    UnerRx_t rx;
    uint32_t reset_time;
    bool comando_listo;
} UnerProtocol_t;

// Inicialización y Máquina de Estados
void Uner_Init(UnerProtocol_t *u);
void Uner_Recibir(UnerProtocol_t *u, uint32_t current_ms);
void Uner_Transmitir(UnerProtocol_t *u);

// Transmisión (TX)
void Uner_AbrirCarga(UnerProtocol_t *u, uint8_t length);
void Uner_Agregar8(UnerProtocol_t *u, uint8_t valor);
void Uner_Agregar16(UnerProtocol_t *u, uint16_t valor);
void Uner_Agregar32(UnerProtocol_t *u, uint32_t valor);
void Uner_CerrarCarga(UnerProtocol_t *u);

// Recepción (RX)
bool Uner_Comando(UnerProtocol_t *u);
uint8_t Uner_IDComando(UnerProtocol_t *u);
uint8_t Uner_Obtener8(UnerProtocol_t *u, uint8_t pos);
uint16_t Uner_Obtener16(UnerProtocol_t *u, uint8_t pos);
uint32_t Uner_Obtener32(UnerProtocol_t *u, uint8_t pos);

#endif /* UNER_PROTOCOL_H_ */