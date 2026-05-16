#ifndef SERIALPORTUNER_H
#define SERIALPORTUNER_H

#include "unerHandler.h"
#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtCore/qiodevice.h>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QElapsedTimer>
#include <QString>

class UnerSerialQT : public UnerHandler {
public:
    UnerSerialQT(QWidget *parent);
    ~UnerSerialQT() = default;

    QSerialPort* Direccion();
    void SetearPuerto(const QString &puerto);
    uint8_t AbrirPuerto(const QString &puerto, qint32 baudRate, QIODeviceBase::OpenMode modo);
    uint8_t Comprobar();
    void CerrarPuerto();
    void ListarPuertos(QComboBox *comboBox);
    QString ObtenerString(uint8_t pos, uint8_t length);

    uint8_t writeable() override;
    void sendByte(uint8_t c) override;
    int32_t readMs() override;
    uint8_t readable() override;
    uint8_t readByte() override;

    void EnviarBufTx() override; // Sobreescritura para enviar en bloque

private:
    QByteArray paraEnviar;
    QSerialPort *PC;          //!< Puerto serie de Qt.
    QElapsedTimer *time;      //!< Objeto para lectura del tiempo.
};

#endif
