#include "unerSerialQT.h"
#include <QDebug>
#include <QEvent>
#include <utility>

UnerSerialQT::UnerSerialQT(QObject *parent) : QObject(parent), UnerHandler() {
    PC = new QSerialPort(this);
    time = new QElapsedTimer;
    time->start();

    // Inicializar los callbacks como vacíos por seguridad
    on_conexion_callback = nullptr;
    on_desconexion_callback = nullptr;

    handshakeTimer = new QTimer(this);
    handshakeTimer->setSingleShot(true);
    connect(handshakeTimer, &QTimer::timeout, this, &UnerSerialQT::onHandshakeTimeout);

    retryTimer = new QTimer(this);
    retryTimer->setSingleShot(true);
    connect(retryTimer, &QTimer::timeout, this, &UnerSerialQT::onRetryTimeout);

    pingPongTimer = new QTimer(this);
    contadorPing = 0;
    pingsPerdidos = 0;
    connect(pingPongTimer, &QTimer::timeout, this, &UnerSerialQT::onPingPongTimeout);

    connect(PC, &QSerialPort::readyRead, this, &UnerSerialQT::onRx);

    retryCount = 0;
    conectado = false;
    combo = nullptr;
    btn = nullptr;
}

// ==========================================
// CONFIGURACIÓN E INYECCIÓN
// ==========================================

void UnerSerialQT::vincularUI(QComboBox *comboPuertos, QPushButton *botonConectar) {
    combo = comboPuertos;
    btn = botonConectar;
    combo->installEventFilter(this);
    ListarPuertos();

    connect(btn, &QPushButton::clicked, this, &UnerSerialQT::onButtonClicked);

    btn->setText("OPEN");
    btn->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");
}

void UnerSerialQT::registrarCallbackConexion(std::function<void()> callback) {
    on_conexion_callback = callback;
}

void UnerSerialQT::registrarCallbackDesconexion(std::function<void()> callback) {
    on_desconexion_callback = callback;
}

QSerialPort* UnerSerialQT::Direccion() { return PC; }
uint8_t UnerSerialQT::Comprobar() { return conectado; }

void UnerSerialQT::ListarPuertos() {
    if(!combo) return;
    combo->clear();

    QList<QSerialPortInfo> puertos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : std::as_const(puertos)) {
        combo->addItem(info.portName());
    }
}

// ==========================================
// MÁQUINA DE ESTADOS Y LÓGICA DE BOTÓN
// ==========================================

void UnerSerialQT::onButtonClicked() {
    if (conectado) {
        cerrarConexionSegura();
    } else {
        if (!combo) return;

        PC->setPortName(combo->currentText());
        PC->setBaudRate(QSerialPort::Baud115200);

        if (PC->open(QIODevice::ReadWrite)) {
            resetHandshakeState();
            btn->setText("WAIT...");
            btn->setStyleSheet("QPushButton { background-color: yellow; color: black; font-weight: bold; }");
            sendHandshakeRequest();
        } else {
            QMessageBox::critical(nullptr, "Error", "No se pudo abrir el puerto serial.");
        }
    }
}

void UnerSerialQT::cerrarConexionSegura() {
    resetHandshakeState();

    if (pingPongTimer->isActive()) pingPongTimer->stop();

    if (PC->isOpen() && PC->isWritable()) {
        AbrirCarga(1);
        AgregarDato((uint8_t)0x16); // CMD_CLOSE
        CerrarCarga();
        EnviarBufTx();
        PC->waitForBytesWritten(100);
        PC->close();
    }

    conectado = false;
    btn->setText("OPEN");
    btn->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");

    // Ejecutar inyección de seguridad/desconexión
    if (on_desconexion_callback) {
        on_desconexion_callback();
    }

    emit conexionPerdida();
}

void UnerSerialQT::resetHandshakeState() {
    if (handshakeTimer->isActive()) handshakeTimer->stop();
    if (retryTimer->isActive()) retryTimer->stop();
    retryCount = 0;
}

void UnerSerialQT::sendHandshakeRequest() {
    AbrirCarga(1);
    AgregarDato((uint8_t)0x01);
    CerrarCarga();
    EnviarBufTx();
    handshakeTimer->start(HANDSHAKE_TIMEOUT_MS);
}

void UnerSerialQT::onHandshakeTimeout() {
    retryCount++;
    if (retryCount < MAX_RETRIES) {
        btn->setText(QString("RETRY %1/%2").arg(retryCount).arg(MAX_RETRIES));
        btn->setStyleSheet("QPushButton { background-color: orange; color: black; font-weight: bold; }");
        retryTimer->start(RETRY_DELAY_MS);
    } else {
        PC->close();
        conectado = false;
        btn->setText("ERROR");
        btn->setStyleSheet("QPushButton { background-color: gray; color: white; font-weight: bold; }");
        QMessageBox::warning(nullptr, "Error de conexión", "No se pudo establecer comunicación (Timeout).");

        QTimer::singleShot(3000, this, [this]() {
            btn->setText("OPEN");
            btn->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");
            resetHandshakeState();
        });
    }
}

void UnerSerialQT::onRetryTimeout() {
    sendHandshakeRequest();
}

// ==========================================
// RECEPCIÓN
// ==========================================

void UnerSerialQT::onRx() {
    Recibir();

    if (Comando()) {
        uint8_t cmdId = IDComando();

        // Podés comentar esta línea cuando ya no necesites tanto debug
        qDebug() << "[UnerSerialQT] RX -> ID:" << Qt::hex << cmdId;

        switch (cmdId) {
        case 0x81: { // Handshake OK
            resetHandshakeState();
            uint8_t status = ObtenerUint8_t(1);

            if (status == 0x00) {
                conectado = true;
                btn->setText("CLOSE");
                btn->setStyleSheet("QPushButton { background-color: green; color: black; font-weight: bold; }");

                contadorPing = 0;
                pingsPerdidos = 0;

                // Desfasaje de 1050ms
                pingPongTimer->start(1050);

                // Ejecutar inyección de inicio de conexión
                if (on_conexion_callback) {
                    on_conexion_callback();
                }

                emit conexionEstablecida();
            } else {
                cerrarConexionSegura();
                QMessageBox::warning(nullptr, "Error", QString("Dispositivo reportó error: 0x%1").arg(status, 2, 16, QChar('0')));
            }
            break;
        }

        case 0x95: { // PONG RECIBIDO DESDE LA PLACA
            pingsPerdidos = 0;
            break;
        }

        default:
            emit nuevoComando(cmdId);
            break;
        }
    }
}

void UnerSerialQT::onPingPongTimeout() {
    pingsPerdidos++;

    if (pingsPerdidos >= 3) {
        qDebug() << "[UnerSerialQT] TIMEOUT: Placa no responde. Forzando desconexión...";
        cerrarConexionSegura();
        return;
    }

    AbrirCarga(2);
    AgregarDato((uint8_t)0x15); // CMD_PING
    AgregarDato(contadorPing);
    CerrarCarga();
    EnviarBufTx();

    contadorPing++;
}

// ==========================================
// IMPLEMENTACIÓN DE MÉTODOS HEREDADOS
// ==========================================

QString UnerSerialQT::ObtenerString(uint8_t pos, uint8_t length) {
    QString stringRecibido = QString::fromUtf8(reinterpret_cast<const char*>(varRx.payload + pos), length);
    QString stringInvertido;
    for (int i = stringRecibido.length() - 1; i >= 0; i--) {
        stringInvertido.append(stringRecibido.at(i));
    }
    return stringInvertido;
}

uint8_t UnerSerialQT::writeable() { return PC->isWritable(); }
void UnerSerialQT::sendByte(uint8_t c) { PC->write(reinterpret_cast<const char*>(&c), 1); }
int32_t UnerSerialQT::readMs() { return time->elapsed(); }
uint8_t UnerSerialQT::readable() { return (PC->bytesAvailable() > 0) && PC->isReadable(); }

uint8_t UnerSerialQT::readByte() {
    char c;
    PC->read(&c, 1);
    return static_cast<uint8_t>(c);
}

void UnerSerialQT::EnviarBufTx() {
    if (varTx.iW > 0 && PC->isWritable()) {
        PC->write(reinterpret_cast<const char*>(varTx.buf), varTx.iW);
        varTx.iW = 0;
        varTx.iR = 0;
    }
}

bool UnerSerialQT::eventFilter(QObject *obj, QEvent *event) {
    if (obj == combo && event->type() == QEvent::MouseButtonPress) {
        QString puertoActual = combo->currentText();
        ListarPuertos();
        int index = combo->findText(puertoActual);
        if (index != -1) {
            combo->setCurrentIndex(index);
        }
    }
    return QObject::eventFilter(obj, event);
}
