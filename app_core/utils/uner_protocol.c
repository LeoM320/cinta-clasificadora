/**
 * @file uner_protocol.c
 * @brief Implementación del motor de protocolo UNER.
 *
 * @details
 * La implementación destaca por su eficiencia en ciclos de CPU y RAM:
 * 1. La recepción (`Uner_Recibir`) procesa los bytes extraídos de `HAL_UART_RxRead()` 
 *    directamente, evaluándolos contra una FSM. Esto elimina la necesidad de copiar
 *    datos de un buffer intermedio al payload, y calcula el Checksum XOR de forma concurrente.
 * 2. Incorpora una lógica de Timeout robusta basada en la inyección de `current_ms` para 
 *    desatascar la máquina de estados frente a bytes perdidos por ruido en la línea.
 * 3. El empaquetado y desempaquetado de datos de 16 y 32 bits asume una serialización 
 *    en formato Little-Endian por defecto.
 */

#include "uner_protocol.h"
#include "../hal/include/hal_uart.h" 

/**
 * @brief Inicializa los contextos de memoria y la máquina de estados del protocolo.
 * 
 * @param[in,out] u Puntero a la instancia del protocolo a inicializar.
 */
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

/**
 * @brief Inicia la construcción de una nueva trama de transmisión.
 * 
 * Escribe el encabezado "UNER", la longitud y el separador en el buffer circular,
 * e inicializa el cálculo del checksum.
 * 
 * @param[in,out] u      Puntero a la instancia del protocolo.
 * @param[in]     length Cantidad de bytes que tendrá el payload (sin incluir cabecera ni checksum).
 */
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

/**
 * @brief Empaqueta un entero de 8 bits en el buffer circular.
 * 
 * @param[in,out] u     Puntero a la instancia del protocolo.
 * @param[in]     valor Dato de 8 bits a transmitir.
 */
void Uner_Agregar8(UnerProtocol_t *u, uint8_t valor) {
    u->tx.buf[u->tx.iW] = valor;
    u->tx.checksum ^= valor;
    u->tx.iW = (u->tx.iW + 1) & UNER_BUFLIMIT;
}

/**
 * @brief Empaqueta un entero de 16 bits en el buffer circular.
 * @note Serializa en formato Little-Endian (Byte menos significativo primero).
 * 
 * @param[in,out] u     Puntero a la instancia del protocolo.
 * @param[in]     valor Dato de 16 bits a transmitir.
 */
void Uner_Agregar16(UnerProtocol_t *u, uint16_t valor) {
    Uner_Agregar8(u, (uint8_t)(valor & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 8) & 0xFF));
}

/**
 * @brief Empaqueta un entero de 32 bits en el buffer circular.
 * @note Serializa en formato Little-Endian.
 * 
 * @param[in,out] u     Puntero a la instancia del protocolo.
 * @param[in]     valor Dato de 32 bits a transmitir.
 */
void Uner_Agregar32(UnerProtocol_t *u, uint32_t valor) {
    Uner_Agregar8(u, (uint8_t)(valor & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 8) & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 16) & 0xFF));
    Uner_Agregar8(u, (uint8_t)((valor >> 24) & 0xFF));
}

/**
 * @brief Finaliza la trama incrustando el byte de paridad (XOR Checksum) calculado.
 * 
 * @param[in,out] u Puntero a la instancia del protocolo.
 */
void Uner_CerrarCarga(UnerProtocol_t *u) {
    u->tx.buf[u->tx.iW] = u->tx.checksum;
    u->tx.iW = (u->tx.iW + 1) & UNER_BUFLIMIT;
}

/**
 * @brief Vacía el buffer circular transmitiendo los bytes pendientes mediante el hardware.
 * @warning Esta función asume que `HAL_UART_TxByte` es bloqueante por cada byte. 
 *          En sistemas con DMA o interrupciones TX, esta lógica podría requerir adaptación.
 * 
 * @param[in,out] u Puntero a la instancia del protocolo.
 */
void Uner_Transmitir(UnerProtocol_t *u) {
    while (u->tx.iR != u->tx.iW) {
        HAL_UART_TxByte(u->tx.buf[u->tx.iR]);
        u->tx.iR = (u->tx.iR + 1) & UNER_BUFLIMIT;
    }
}

// ==========================================
// SECCIÓN DE RECEPCIÓN: ZERO-COPY PARSING
// ==========================================

/**
 * @brief Evalúa los datos entrantes de la UART mediante una máquina de estados (FSM).
 * 
 * Esta función extrae todos los bytes disponibles en los registros de la UART 
 * de manera no bloqueante. Si se detecta un estancamiento prolongado (Timeout), 
 * la máquina de estados se reinicia para prevenir bloqueos por tramas corruptas.
 * 
 * @param[in,out] u          Puntero a la instancia del protocolo.
 * @param[in]     current_ms Tiempo del sistema en milisegundos (típicamente desde SysTick).
 */
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

/**
 * @brief Consulta si hay un comando nuevo validado disponible.
 * 
 * Tras ser leída como `true`, la bandera interna se restablece para prevenir 
 * lecturas duplicadas de la misma trama.
 * 
 * @param[in,out] u Puntero a la instancia del protocolo.
 * @return true     Si hay una trama válida en `payload`.
 * @return false    Si no hay tramas nuevas.
 */
bool Uner_Comando(UnerProtocol_t *u) {
    if (u->comando_listo) {
        u->comando_listo = false;
        return true;
    }
    return false;
}

/**
 * @brief Obtiene el byte inicial del payload, usualmente tratado como el ID del comando.
 * 
 * @param[in] u Puntero a la instancia del protocolo.
 * @return uint8_t ID o primer byte de la carga útil.
 */
uint8_t Uner_IDComando(UnerProtocol_t *u) { return u->rx.payload[0]; }

/**
 * @brief Extrae un dato de 8 bits del payload.
 * 
 * @param[in] u   Puntero a la instancia del protocolo.
 * @param[in] pos Índice dentro del buffer `payload`.
 * @return uint8_t Dato extraído.
 */
uint8_t Uner_Obtener8(UnerProtocol_t *u, uint8_t pos) { return u->rx.payload[pos]; }

/**
 * @brief Extrae y ensambla un dato de 16 bits desde el payload.
 * @note Asume que los bytes llegaron en formato Little-Endian.
 * 
 * @param[in] u   Puntero a la instancia del protocolo.
 * @param[in] pos Índice inicial dentro del buffer `payload`.
 * @return uint16_t Dato de 16 bits ensamblado.
 */
uint16_t Uner_Obtener16(UnerProtocol_t *u, uint8_t pos) {
    return (uint16_t)u->rx.payload[pos] | ((uint16_t)u->rx.payload[pos + 1] << 8);
}

/**
 * @brief Extrae y ensambla un dato de 32 bits desde el payload.
 * @note Asume que los bytes llegaron en formato Little-Endian.
 * 
 * @param[in] u   Puntero a la instancia del protocolo.
 * @param[in] pos Índice inicial dentro del buffer `payload`.
 * @return uint32_t Dato de 32 bits ensamblado.
 */
uint32_t Uner_Obtener32(UnerProtocol_t *u, uint8_t pos) {
    return (uint32_t)u->rx.payload[pos] | 
          ((uint32_t)u->rx.payload[pos + 1] << 8) |
          ((uint32_t)u->rx.payload[pos + 2] << 16) |
          ((uint32_t)u->rx.payload[pos + 3] << 24);
}