#ifndef SERIALPORTUNER_H
#define SERIALPORTUNER_H

#include "unerHandler.h"
#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QPushButton>
#include <QElapsedTimer>
#include <QTimer>
#include <QMessageBox>
#include <QString>
#include <functional>

// CONSTANTES DEL HANDSHAKE
#define MAX_RETRIES 3
#define HANDSHAKE_TIMEOUT_MS 500
#define RETRY_DELAY_MS 500

class UnerSerialQT : public QObject, public UnerHandler {
    Q_OBJECT

public:
    explicit UnerSerialQT(QObject *parent = nullptr);
    ~UnerSerialQT() = default;

    // Vinculación visual
    void vincularUI(QComboBox *comboPuertos, QPushButton *botonConectar);

    // Inyección de Seguridad y Eventos (Callbacks)
    void registrarCallbackConexion(std::function<void()> callback);
    void registrarCallbackDesconexion(std::function<void()> callback);

    // Métodos originales
    QSerialPort* Direccion();
    uint8_t Comprobar();
    void ListarPuertos();
    QString ObtenerString(uint8_t pos, uint8_t length);

    uint8_t writeable() override;
    void sendByte(uint8_t c) override;
    int32_t readMs() override;
    uint8_t readable() override;
    uint8_t readByte() override;
    void EnviarBufTx() override;

    void cerrarConexionSegura();

signals:
    void conexionEstablecida();
    void conexionPerdida();
    void nuevoComando(uint8_t cmdId);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onButtonClicked();
    void onRx();
    void onHandshakeTimeout();
    void onRetryTimeout();
    void onPingPongTimeout();

private:
    void sendHandshakeRequest();
    void resetHandshakeState();

    QSerialPort *PC;
    QElapsedTimer *time;

    QComboBox *combo;
    QPushButton *btn;

    QTimer *handshakeTimer;
    QTimer *retryTimer;
    QTimer *pingPongTimer;
    uint8_t contadorPing;
    uint8_t pingsPerdidos;

    int retryCount;
    bool conectado;

    // Variables para guardar las rutinas inyectadas
    std::function<void()> on_conexion_callback;
    std::function<void()> on_desconexion_callback;
};

#endif // SERIALPORTUNER_H
