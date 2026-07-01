#include "unerHandler.h"

UnerHandler::UnerHandler() {
    varTx.iR = 0;
    varTx.iW = 0;
    varRx.state = U;
    reset = 0;
}

void UnerHandler::AbrirCarga(uint8_t length) {
    varTx.buf[varTx.iW++] = 'U';
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.buf[varTx.iW++] = 'N';
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.buf[varTx.iW++] = 'E';
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.buf[varTx.iW++] = 'R';
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.buf[varTx.iW++] = length + 1;
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.buf[varTx.iW++] = ':';
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;

    varTx.checksum = 'U' ^ 'N' ^ 'E' ^ 'R' ^ (length + 1) ^ ':';
}

void UnerHandler::AgregarDato(uint8_t valor) {
    varTx.buf[varTx.iW] = valor;
    varTx.checksum ^= varTx.buf[varTx.iW++];
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;
}

void UnerHandler::AgregarDato(uint16_t valor) {
    u16.valor = valor;
    for (int i = 0; i < 2; i++) {
        varTx.buf[varTx.iW] = u16.byte[i];
        varTx.checksum ^= varTx.buf[varTx.iW++];
        if (varTx.iW > BUFLIMIT) varTx.iW = 0;
    }
}

void UnerHandler::AgregarDato(uint32_t valor) {
    u32.valor = valor;
    for (int i = 0; i < 4; i++) {
        varTx.buf[varTx.iW] = u32.byte[i];
        varTx.checksum ^= varTx.buf[varTx.iW++];
        if (varTx.iW > BUFLIMIT) varTx.iW = 0;
    }
}

void UnerHandler::AgregarDato(float valor) {
    uF.valor = valor;
    for (int i = 0; i < 4; i++) {
        varTx.buf[varTx.iW] = uF.byte[i];
        varTx.checksum ^= varTx.buf[varTx.iW++];
        if (varTx.iW > BUFLIMIT) varTx.iW = 0;
    }
}

void UnerHandler::AgregarDato(uint8_t *valor, uint8_t length) {
    for (int i = 0; i < length; i++) {
        varTx.buf[varTx.iW] = valor[length - i - 1]; // Inversión solicitada
        varTx.checksum ^= varTx.buf[varTx.iW++];
        if (varTx.iW > BUFLIMIT) varTx.iW = 0;
    }
}

void UnerHandler::CerrarCarga() {
    varTx.buf[varTx.iW++] = varTx.checksum;
    if (varTx.iW > BUFLIMIT) varTx.iW = 0;
    Transmitir();
}

void UnerHandler::Transmitir() {
    while (writeable() && (varTx.iR != varTx.iW)) {
        sendByte(varTx.buf[varTx.iR++]);
        if (varTx.iR > BUFLIMIT) varTx.iR = 0;
    }
}

void UnerHandler::Recibir() {
    if ((readMs() - reset) > REFRESH) {
        varRx.state = U;
    }
    reset = readMs();

    if (varRx.iW > BUFLIMIT) varRx.iW = 0;
    if (varRx.iR > BUFLIMIT) varRx.iR = 0;

    while (readable()) {
        varRx.buf[varRx.iW++] = readByte();
        if (varRx.iW > BUFLIMIT) varRx.iW = 0;
        Decodificar();
    }
}

void UnerHandler::Decodificar() {
    uint8_t auxInt = varRx.iW;
    while (varRx.iR != auxInt) {
        switch (varRx.state) {
        case U:
            if (varRx.buf[varRx.iR] == 'U') {
                varRx.checksum = varRx.buf[varRx.iR];
                varRx.state = N;
            }
            varRx.iR++;
            break;
        case N:
            if (varRx.buf[varRx.iR] == 'N') {
                varRx.checksum ^= varRx.buf[varRx.iR];
                varRx.state = E;
            } else {
                varRx.state = U;
                varRx.iR--;
            }
            varRx.iR++;
            break;
        case E:
            if (varRx.buf[varRx.iR] == 'E') {
                varRx.checksum ^= varRx.buf[varRx.iR];
                varRx.state = R;
            } else {
                varRx.state = U;
                varRx.iR--;
            }
            varRx.iR++;
            break;
        case R:
            if (varRx.buf[varRx.iR] == 'R') {
                varRx.checksum ^= varRx.buf[varRx.iR];
                varRx.state = LENGTH;
            } else {
                varRx.state = U;
                varRx.iR--;
            }
            varRx.iR++;
            break;
        case LENGTH:
            varRx.length = varRx.buf[varRx.iR];
            varRx.checksum ^= varRx.buf[varRx.iR];
            varRx.state = TOKEN;
            varRx.iR++;
            break;
        case TOKEN:
            if (varRx.buf[varRx.iR] == ':') {
                varRx.checksum ^= varRx.buf[varRx.iR];
                varRx.payloadCount = 0;
                varRx.state = PAYLOAD;
            } else {
                varRx.state = U;
                varRx.iR--;
            }
            varRx.iR++;
            break;
        case PAYLOAD:
            if (varRx.payloadCount < (varRx.length - 1)) {
                varRx.checksum ^= varRx.buf[varRx.iR];
                varRx.payload[varRx.payloadCount] = varRx.buf[varRx.iR];
                varRx.payloadCount++;
                varRx.iR++;
            } else {
                varRx.state = CHECKSUM;
            }
            break;
        case CHECKSUM:
            if (varRx.checksum == varRx.buf[varRx.iR++]) {
                comando = 1;
            }
            varRx.state = U;
            break;
        }
        if (varRx.iR > BUFLIMIT) {
            varRx.iR = 0;
        }
    }
}

uint8_t UnerHandler::Comando() {
    if (comando) {
        comando = 0;
        return 1;
    }
    return 0;
}

uint8_t UnerHandler::IDComando() {
    return varRx.payload[0];
}

uint8_t UnerHandler::ObtenerUint8_t(uint8_t pos) {
    return varRx.payload[pos];
}

uint16_t UnerHandler::ObtenerUint16_t(uint8_t pos) {
    for (int i = 0; i < 2; i++) u16.byte[i] = varRx.payload[pos + i];
    return u16.valor;
}

uint32_t UnerHandler::ObtenerUint32_t(uint8_t pos) {
    for (int i = 0; i < 4; i++) u32.byte[i] = varRx.payload[pos + i];
    return u32.valor;
}

float UnerHandler::ObtenerFloat(uint8_t pos) {
    for (int i = 0; i < 4; i++) uF.byte[i] = varRx.payload[pos + i];
    return uF.valor;
}

void UnerHandler::EnviarBufTx() {
    /*for (int i = 0; i < BUFSIZE; i++) {
        sendByte(varTx.buf[i]);
    }*/
}

void UnerHandler::EnviarBufRx() {
    /*for (int i = 0; i < BUFSIZE; i++) {
        sendByte(varRx.buf[i]);
    }*/
}
