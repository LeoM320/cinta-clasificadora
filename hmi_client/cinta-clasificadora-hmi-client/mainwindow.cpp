#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQmlContext>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    puertoSerie = new UnerSerialQT(this);

    // ==========================================
    // 1. INYECCIÓN DEL PROTOCOLO DE CONEXIÓN
    // ==========================================
    puertoSerie->registrarCallbackConexion([this]() {
        qDebug() << "\n==================================================";
        qDebug() << "✅ [QT CALLBACK] EVENTO: CONEXIÓN ESTABLECIDA";
        qDebug() << "   -> Iniciando motor de telemetría a 125ms...";
        qDebug() << "==================================================\n";

        emit connectionStatusChanged(true);
        timerPolling->start(500);
    });

    // ==========================================
    // 2. INYECCIÓN DEL PROTOCOLO DE EMERGENCIA
    // ==========================================
    puertoSerie->registrarCallbackDesconexion([this]() {
        qDebug() << "\n==================================================";
        qDebug() << "🚨 [QT CALLBACK] EVENTO: CONEXIÓN PERDIDA/CORTADA";
        qDebug() << "   -> Deteniendo telemetría por seguridad...";
        qDebug() << "==================================================\n";

        timerPolling->stop();
        emit connectionStatusChanged(false);
    });

    // ==========================================
    // 3. VINCULACIÓN DE SEÑALES Y UI
    // ==========================================
    puertoSerie->vincularUI(ui->comboBoxPuertos, ui->pushButtonOpenClose);
    connect(puertoSerie, &UnerSerialQT::nuevoComando, this, &MainWindow::onRx);

    // ==========================================
    // 4. CONFIGURACIÓN DE TIMERS Y QML
    // ==========================================
    timerPolling = new QTimer(this);
    connect(timerPolling, &QTimer::timeout, this, &MainWindow::onTimerPolling);

    ui->visorQml->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->visorQml->rootContext()->setContextProperty("backend", this);
    ui->visorQml->setSource(QUrl("qrc:/interfaz.qml"));
}

MainWindow::~MainWindow() {
    delete ui;
}

// ==========================================
// SLOTS DE CONTROL (QML -> C++)
// ==========================================

void MainWindow::encenderCinta() {
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x01); // 1 = Encender
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::apagarCinta() {
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x00); // 0 = Apagar
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::setServo(int servoId, int angulo) {
    if (!puertoSerie->Comprobar()) return;
    if (servoId < 0 || servoId > 2) return;
    if (angulo < 0 || angulo > 180) return;

    puertoSerie->AbrirCarga(3);
    puertoSerie->AgregarDato((uint8_t)0x03); // CMD_SET_SERVO
    puertoSerie->AgregarDato((uint8_t)servoId);
    puertoSerie->AgregarDato((uint8_t)angulo);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// ==========================================
// TELEMETRÍA Y POLLING
// ==========================================

void MainWindow::requestDistance() {
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(1);
    puertoSerie->AgregarDato((uint8_t)0x04);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::requestIrStates() {
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(1);
    puertoSerie->AgregarDato((uint8_t)0x05);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::onTimerPolling() {
    if (!puertoSerie->Comprobar()) return;

    // Ping-Pong de peticiones de datos
    static bool flag_alternar = false;
    if (flag_alternar) {
        requestDistance();
    } else {
        requestIrStates();
    }
    flag_alternar = !flag_alternar;
}

// ==========================================
// RECEPCIÓN DE DATOS DEL MICRO
// ==========================================

void MainWindow::onRx(uint8_t cmdId) {
    switch (cmdId) {

    case 0x84: // ACK_GET_DISTANCE
        emit distanceUpdated(puertoSerie->ObtenerUint16_t(1));
        break;

    case 0x85: // ACK_GET_IR_STATES
    {
        uint8_t irPack = puertoSerie->ObtenerUint8_t(1);
        emit irStatesUpdated((irPack & 1) != 0, (irPack & 2) != 0, (irPack & 4) != 0, (irPack & 8) != 0);
        break;
    }

    case 0x09: // CMD_SEND_LOG
    {
        QByteArray textBytes;
        int i = 1;

        while (true) {
            uint8_t letra = puertoSerie->ObtenerUint8_t(i);
            if (letra == '\0' || i > 65) {
                break;
            }
            textBytes.append((char)letra);
            i++;
        }

        QString logStr = QString::fromLatin1(textBytes);
        qDebug() << "LOG: " <<logStr;
        emit logMessageReceived(logStr);
        break;
    }

    case 0x86: // ACK_SET_BELT
    case 0x83: // ACK_SET_SERVO
        break;

    default:
        qDebug() << "[MainWindow] RX Desconocido o No Manejado:" << Qt::hex << cmdId;
        break;
    }
}
