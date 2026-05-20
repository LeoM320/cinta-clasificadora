#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQmlContext>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    puertoSerie = new UnerSerialQT(this);
    connect(puertoSerie->Direccion(), &QSerialPort::readyRead, this, &MainWindow::onRx);
    puertoSerie->ListarPuertos(ui->comboBoxPuertos);

    // Configuración del Timer de Polling (Ejecutará onTimerPolling cada vez que venza)
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

// Bucle Continuo de Peticiones
void MainWindow::onTimerPolling() {
    if (!puertoSerie->Comprobar()) return;

    // Usamos una variable estática para alternar las peticiones (Ping-Pong)
    static bool flag_alternar = false;

    if (flag_alternar) {
        requestDistance();
    } else {
        requestIrStates();
    }

    flag_alternar = !flag_alternar;
}

void MainWindow::onRx() {
    puertoSerie->Recibir();
    if (puertoSerie->Comando()) {
        uint8_t cmdId = puertoSerie->IDComando();

        switch (cmdId) {
        case 0x81:
            ui->pushButtonOpenClose->setText("CLOSE");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: green; color: black; font-weight: bold; }");
            emit connectionStatusChanged(true);

            // EL MICRO RESPONDIÓ. INICIAMOS LA METRALLA DE TELEMETRÍA (4 veces por segundo)
            timerPolling->start(750);
            break;

        case 0x84:
            emit distanceUpdated(puertoSerie->ObtenerUint16_t(1));
            break;

        case 0x85:
        {
            uint8_t irPack = puertoSerie->ObtenerUint8_t(1);
            emit irStatesUpdated((irPack & 1) != 0, (irPack & 2) != 0, (irPack & 4) != 0, (irPack & 8) != 0);
            break;
        }

            // NUEVO CASE PARA EL LOG (0x09)
        case 0x09:
        {
            QByteArray textBytes;
            int i = 1; // Empezamos en el índice 1 (el 0 es el ID del comando)

            while (true) {
                uint8_t letra = puertoSerie->ObtenerUint8_t(i);

                // Si encontramos el terminador nulo o pasamos el límite de seguridad, cortamos
                if (letra == '\0' || i > 65) {
                    break;
                }

                textBytes.append((char)letra);
                i++;
            }

            QString logStr = QString::fromLatin1(textBytes);
            emit logMessageReceived(logStr);
            break;
        }

        case 0x86: case 0x83: break;
        default: qDebug() << "RX Desconocido:" << Qt::hex << cmdId; break;
        }
    }
}

void MainWindow::on_pushButtonOpenClose_clicked() {
    if (puertoSerie->Comprobar()) {
        timerPolling->stop(); // FRENAR EL POLLING ANTES DE CERRAR
        emit connectionStatusChanged(false);
        QTimer::singleShot(50, this, [this]() {
            puertoSerie->CerrarPuerto();
            ui->pushButtonOpenClose->setText("OPEN");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");
        });
    } else {
        if (puertoSerie->AbrirPuerto(ui->comboBoxPuertos->currentText(), QSerialPort::Baud115200, QIODevice::ReadWrite)) {
            ui->pushButtonOpenClose->setText("WAIT...");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: yellow; color: black; font-weight: bold; }");

            puertoSerie->AbrirCarga(1);
            puertoSerie->AgregarDato((uint8_t)0x01);
            puertoSerie->CerrarCarga();
            puertoSerie->EnviarBufTx();
        }
    }
}
