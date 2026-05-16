/**
 * @file uner_protocol.c
 * @brief Implementación del motor UNER.
 */

#include "uner_protocol.h"
#include "../hal/include/hal_uart.h" // Conexión directa con tu driver

void Uner_Init(UnerProtocol_t *u) {
    u->tx.iR = 0;
    u->tx.iW = 0;
    u->rx.iR = 0;
    u->rx.iW = 0;
    u->rx.state = UNER_STATE_U;
    u->reset_time = 0;
    u->comando_listo = false;
}

// ==========================================
// SECCIÓN DE TRANSMISIÓN (TX)
// ==========================================
void Uner_AbrirCarga(UnerProtocol_t *u, uint8_t length) {
    UnerTx_t *tx = &u->tx;
    tx->buf[tx->iW] = 'U'; tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    tx->buf[tx->iW] = 'N'; tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    tx->buf[tx->iW] = 'E'; tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    tx->buf[tx->iW] = 'R'; tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    
    tx->buf[tx->iW] = length + 1; tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    tx->buf[tx->iW] = ':';        tx->iW = (tx->iW + 1) & UNER_BUFLIMIT;
    
    tx->checksum = 'U' ^ 'N' ^ 'E' ^ 'R' ^ (length + 1) ^ ':';
}

void Uner_Agregar8(UnerProtocol_t *u, uint8_t valor) {
    u->tx.buf[u->tx.iW] = valor;
    u->tx.checksum ^= valor;
    u->tx.iW = (u->tx.iW + 1) & UNER_BUFLIMIT;
}

void Uner_Agregar16(UnerProtocol_t *u, uint16_t valor) {
    Uner_Agregar8(u, (uint8_t)(valor & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 8) & 0xFF));
}

void Uner_Agregar32(UnerProtocol_t *u, uint32_t valor) {
    Uner_Agregar8(u, (uint8_t)(valor & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 8) & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 16) & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 24) & 0xFF));
}

void Uner_CerrarCarga(UnerProtocol_t *u) {
    u->tx.buf[u->tx.iW] = u->tx.checksum;
    u->tx.iW = (u->tx.iW + 1) & UNER_BUFLIMIT;
}

void Uner_Transmitir(UnerProtocol_t *u) {
    // Si hay datos en el buffer, se despachan por hardware
    while (u->tx.iR != u->tx.iW) {
        HAL_UART_TxByte(u->tx.buf[u->tx.iR]);
        u->tx.iR = (u->tx.iR + 1) & UNER_BUFLIMIT;
    }
}

// ==========================================
// SECCIÓN DE RECEPCIÓN Y DECODIFICACIÓN (RX)
// ==========================================
void Uner_Recibir(UnerProtocol_t *u, uint32_t current_ms) {
    // Timeout de reseteo
    if ((current_ms - u->reset_time) > UNER_REFRESH_MS) {
        u->rx.state = UNER_STATE_U;
    }
    u->reset_time = current_ms;

    // Volcar hardware al buffer circular del parser
    while (HAL_UART_RxDataAvailable()) {
        u->rx.buf[u->rx.iW] = HAL_UART_RxRead();
        u->rx.iW = (u->rx.iW + 1) & UNER_BUFLIMIT;
        Uner_Decodificar(u);
    }
}

void Uner_Decodificar(UnerProtocol_t *u) {
    UnerRx_t *rx = &u->rx;
    uint8_t limit = rx->iW;
    
    while (rx->iR != limit) {
        uint8_t byte = rx->buf[rx->iR];
        
        switch(rx->state) {
            case UNER_STATE_U:
                if (byte == 'U') { rx->checksum = byte; rx->state = UNER_STATE_N; }
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_N:
                if (byte == 'N') { rx->checksum ^= byte; rx->state = UNER_STATE_E; }
                else { rx->state = UNER_STATE_U; rx->iR = (rx->iR - 1) & UNER_BUFLIMIT; }
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_E:
                if (byte == 'E') { rx->checksum ^= byte; rx->state = UNER_STATE_R; }
                else { rx->state = UNER_STATE_U; rx->iR = (rx->iR - 1) & UNER_BUFLIMIT; }
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_R:
                if (byte == 'R') { rx->checksum ^= byte; rx->state = UNER_STATE_LENGTH; }
                else { rx->state = UNER_STATE_U; rx->iR = (rx->iR - 1) & UNER_BUFLIMIT; }
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_LENGTH:
                rx->length = byte;
                rx->checksum ^= byte;
                rx->state = UNER_STATE_TOKEN;
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_TOKEN:
                if (byte == ':') {
                    rx->checksum ^= byte;
                    rx->payloadCount = 0;
                    rx->state = UNER_STATE_PAYLOAD;
                } else {
                    rx->state = UNER_STATE_U; rx->iR = (rx->iR - 1) & UNER_BUFLIMIT;
                }
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
            case UNER_STATE_PAYLOAD:
                if (rx->payloadCount < (rx->length - 1)) {
                    rx->checksum ^= byte;
                    rx->payload[rx->payloadCount++] = byte;
                    rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                } else {
                    rx->state = UNER_STATE_CHECKSUM;
                }
                break;
            case UNER_STATE_CHECKSUM:
                if (rx->checksum == byte) { u->comando_listo = true; }
                rx->state = UNER_STATE_U;
                rx->iR = (rx->iR + 1) & UNER_BUFLIMIT;
                break;
        }
    }
}

// ==========================================
// EXTRACCIÓN DE DATOS
// ==========================================
bool Uner_Comando(UnerProtocol_t *u) {
    if (u->comando_listo) {
        u->comando_listo = false;
        return true;
    }
    return false;
}

uint8_t Uner_IDComando(UnerProtocol_t *u) { return u->rx.payload[0]; }

uint8_t Uner_Obtener8(UnerProtocol_t *u, uint8_t pos) { return u->rx.payload[pos]; }

uint16_t Uner_Obtener16(UnerProtocol_t *u, uint8_t pos) {
    return (uint16_t)u->rx.payload[pos] | ((uint16_t)u->rx.payload[pos + 1] << 8);
}

uint32_t Uner_Obtener32(UnerProtocol_t *u, uint8_t pos) {
    return (uint32_t)u->rx.payload[pos] | 
          ((uint32_t)u->rx.payload[pos + 1] << 8) |
          ((uint32_t)u->rx.payload[pos + 2] << 16) |
          ((uint32_t)u->rx.payload[pos + 3] << 24);
}