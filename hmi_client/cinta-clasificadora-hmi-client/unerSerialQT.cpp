#include "unerSerialQT.h"

UnerSerialQT::UnerSerialQT(QWidget *parent) : UnerHandler() {
    PC = new QSerialPort(parent);
    time = new QElapsedTimer;
    time->start(); // INICIO DEL TIMER: Solución al bug del REFRESH
}

QSerialPort* UnerSerialQT::Direccion() {
    return PC;
}

void UnerSerialQT::SetearPuerto(const QString &puerto) {
    PC->setPortName(puerto);
}

uint8_t UnerSerialQT::AbrirPuerto(const QString &puerto, qint32 baudRate, QIODeviceBase::OpenMode modo) {
    PC->setPortName(puerto);
    PC->setBaudRate(baudRate);
    return PC->open(modo);
}

uint8_t UnerSerialQT::Comprobar() {
    return PC->isOpen();
}

void UnerSerialQT::CerrarPuerto() {
    PC->close();
}

void UnerSerialQT::ListarPuertos(QComboBox *comboBox) {
    comboBox->clear();
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfoList) {
        comboBox->addItem(serialPortInfo.portName());
    }
}

QString UnerSerialQT::ObtenerString(uint8_t pos, uint8_t length) {
    // Al ser protected en la clase base, ahora sí tenemos acceso a varRx.payload
    QString stringRecibido = QString::fromUtf8(reinterpret_cast<const char*>(varRx.payload + pos), length);
    QString stringInvertido;
    for (int i = stringRecibido.length() - 1; i >= 0; i--) {
        stringInvertido.append(stringRecibido.at(i));
    }
    return stringInvertido;
}

uint8_t UnerSerialQT::writeable() {
    return PC->isWritable();
}

void UnerSerialQT::sendByte(uint8_t c) {
    PC->write(reinterpret_cast<const char*>(&c), sizeof(c));
}

int32_t UnerSerialQT::readMs() {
    return time->elapsed();
}

uint8_t UnerSerialQT::readable() {
    return (PC->bytesAvailable() > 0) && PC->isReadable();
}

uint8_t UnerSerialQT::readByte() {
    // Leemos 1 byte de forma segura
    char c;
    PC->read(&c, 1);
    return static_cast<uint8_t>(c);
}

void UnerSerialQT::EnviarBufTx() {
    // OPTIMIZACIÓN: Enviamos todo el bloque de memoria de una vez usando el SO
    // En lugar de enviar la petición byte a byte
    if (varTx.iW > 0 && PC->isWritable()) {
        PC->write(reinterpret_cast<const char*>(varTx.buf), varTx.iW);
        // Al transmitirse de golpe, reseteamos el buffer
        varTx.iW = 0;
        varTx.iR = 0;
    }
}
