#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQmlContext>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    puertoSerie = new UnerSerialQT(this);
    connect(puertoSerie->Direccion(), &QSerialPort::readyRead, this, &MainWindow::onRx);
    puertoSerie->ListarPuertos(ui->comboBoxPuertos);

    ui->visorQml->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->visorQml->rootContext()->setContextProperty("backend", this);
    ui->visorQml->setSource(QUrl("qrc:/interfaz.qml"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==========================================
// SLOTS DE CONTROL (QML -> C++)
// ==========================================

void MainWindow::encenderCinta()
{
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x01); // 1 = Encender
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::apagarCinta()
{
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x00); // 0 = Apagar
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::setServo(int servoId, int angulo)
{
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

void MainWindow::requestDistance()
{
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(1);
    puertoSerie->AgregarDato((uint8_t)0x04); // CMD_GET_DISTANCE
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::requestIrStates()
{
    if (!puertoSerie->Comprobar()) return;
    puertoSerie->AbrirCarga(1);
    puertoSerie->AgregarDato((uint8_t)0x05); // CMD_GET_IR_STATES
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// ==========================================
// RECEPCIÓN DE DATOS (C++ -> QML)
// ==========================================

void MainWindow::onRx()
{
    puertoSerie->Recibir();

    if (puertoSerie->Comando()) {
        uint8_t cmdId = puertoSerie->IDComando();

        switch (cmdId) {
        case 0x81: // ACK_ALIVE
        {
            // uint32_t uptime = puertoSerie->ObtenerUint32_t(1); // Opcional
            ui->pushButtonOpenClose->setText("CLOSE");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: green; color: black; font-weight: bold; }");
            emit connectionStatusChanged(true); // Avisar a QML que estamos listos
            break;
        }
        case 0x84: // ACK_GET_DISTANCE
        {
            uint16_t dist = puertoSerie->ObtenerUint16_t(1);
            emit distanceUpdated(dist);
            break;
        }
        case 0x85: // ACK_GET_IR_STATES
        {
            uint8_t irPack = puertoSerie->ObtenerUint8_t(1);
            // Desempaquetado de bits (Bit-Unpacking)
            bool ir0 = (irPack & (1 << 0)) != 0;
            bool ir1 = (irPack & (1 << 1)) != 0;
            bool ir2 = (irPack & (1 << 2)) != 0;
            bool ir3 = (irPack & (1 << 3)) != 0;
            emit irStatesUpdated(ir0, ir1, ir2, ir3);
            break;
        }
        case 0x86: // ACK_SET_BELT
        case 0x83: // ACK_SET_SERVO
            // Confirmaciones silenciosas, se pueden omitir del log si se desea
            break;
        default:
            qDebug() << "RX Desconocido o Error:" << Qt::hex << cmdId;
            break;
        }
    }
}

// ==========================================
// BOTÓN CONEXIÓN
// ==========================================

void MainWindow::on_pushButtonOpenClose_clicked()
{
    if (puertoSerie->Comprobar()) {
        // --- DESCONEXIÓN ---
        emit connectionStatusChanged(false); // Detener el polling en QML
        QTimer::singleShot(50, this, [this]() {
            puertoSerie->CerrarPuerto();
            ui->pushButtonOpenClose->setText("OPEN");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");
        });
    } else {
        // --- CONEXIÓN ---
        if (puertoSerie->AbrirPuerto(ui->comboBoxPuertos->currentText(),
                                     QSerialPort::Baud115200,
                                     QIODevice::ReadWrite)) {
            ui->pushButtonOpenClose->setText("WAIT...");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: yellow; color: black; font-weight: bold; }");

            // Enviar Petición ALIVE (0x01)
            puertoSerie->AbrirCarga(1);
            puertoSerie->AgregarDato((uint8_t)0x01);
            puertoSerie->CerrarCarga();
            puertoSerie->EnviarBufTx();
        }
    }
}

void MainWindow::iniciarMotorDesdeC() {}
void MainWindow::configurarHeartbeat(bool, int) {}
void MainWindow::onTimer() {}
