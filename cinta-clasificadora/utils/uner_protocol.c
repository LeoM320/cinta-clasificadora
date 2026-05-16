/**
 * @file uner_protocol.c
 * @brief Implementación del motor UNER con decodificación On-The-Fly.
 */

#include "uner_protocol.h"
#include "../hal/include/hal_uart.h" 

void Uner_Init(UnerProtocol_t *u) {
    u->tx.iR = 0;
    u->tx.iW = 0;
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
    while (u->tx.iR != u->tx.iW) {
        HAL_UART_TxByte(u->tx.buf[u->tx.iR]);
        u->tx.iR = (u->tx.iR + 1) & UNER_BUFLIMIT;
    }
}

// ==========================================
// SECCIÓN DE RECEPCIÓN: ZERO-COPY PARSING
// ==========================================
void Uner_Recibir(UnerProtocol_t *u, uint32_t current_ms) {
    // Control de timeout: Si pasa mucho tiempo sin recibir, reiniciamos la máquina.
    if ((current_ms - u->reset_time) > UNER_REFRESH_MS) {
        u->rx.state = UNER_STATE_U;
    }
    u->reset_time = current_ms;

    // Leemos byte por byte directo del hardware y lo procesamos al vuelo
    while (HAL_UART_RxDataAvailable()) {
        uint8_t byte = HAL_UART_RxRead();
        
        switch(u->rx.state) {
            case UNER_STATE_U:
                if (byte == 'U') { u->rx.checksum = byte; u->rx.state = UNER_STATE_N; }
                break;
                
            case UNER_STATE_N:
                if (byte == 'N') { u->rx.checksum ^= byte; u->rx.state = UNER_STATE_E; }
                else if (byte == 'U') { u->rx.checksum = byte; } // Falsa alarma, inicia nueva cabecera
                else { u->rx.state = UNER_STATE_U; }
                break;
                
            case UNER_STATE_E:
                if (byte == 'E') { u->rx.checksum ^= byte; u->rx.state = UNER_STATE_R; }
                else if (byte == 'U') { u->rx.checksum = byte; u->rx.state = UNER_STATE_N; }
                else { u->rx.state = UNER_STATE_U; }
                break;
                
            case UNER_STATE_R:
                if (byte == 'R') { u->rx.checksum ^= byte; u->rx.state = UNER_STATE_LENGTH; }
                else if (byte == 'U') { u->rx.checksum = byte; u->rx.state = UNER_STATE_N; }
                else { u->rx.state = UNER_STATE_U; }
                break;
                
            case UNER_STATE_LENGTH:
                u->rx.length = byte;
                u->rx.checksum ^= byte;
                u->rx.state = UNER_STATE_TOKEN;
                break;
                
            case UNER_STATE_TOKEN:
                if (byte == ':') {
                    u->rx.checksum ^= byte;
                    u->rx.payloadCount = 0;
                    
                    // Verificación de seguridad extra: Si el length es 0, no hay payload
                    if (u->rx.length > 0) {
                        u->rx.state = UNER_STATE_PAYLOAD;
                    } else {
                        u->rx.state = UNER_STATE_CHECKSUM;
                    }
                } 
                else if (byte == 'U') { u->rx.checksum = byte; u->rx.state = UNER_STATE_N; }
                else { u->rx.state = UNER_STATE_U; }
                break;
                
            case UNER_STATE_PAYLOAD:
                if (u->rx.payloadCount < (u->rx.length - 1)) {
                    u->rx.checksum ^= byte;
                    u->rx.payload[u->rx.payloadCount++] = byte;
                }
                
                // Si ya completamos la cantidad requerida, el próximo estado es el Checksum
                if (u->rx.payloadCount == (u->rx.length - 1)) {
                    u->rx.state = UNER_STATE_CHECKSUM;
                }
                break;
                
            case UNER_STATE_CHECKSUM:
                if (u->rx.checksum == byte) { 
                    u->comando_listo = true; 
                }
                u->rx.state = UNER_STATE_U;
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