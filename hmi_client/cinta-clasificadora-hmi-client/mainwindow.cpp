#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQmlContext>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Inicializamos el puerto serie y conectamos la recepción
    puertoSerie = new UnerSerialQT(this);
    connect(puertoSerie->Direccion(), &QSerialPort::readyRead, this, &MainWindow::onRx);
    puertoSerie->ListarPuertos(ui->comboBoxPuertos);

    // 1. Configurar que el QML se adapte al tamaño del widget contenedor
    ui->visorQml->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // 2. Inyectar 'this' (MainWindow) al QML con el nombre "backend"
    ui->visorQml->rootContext()->setContextProperty("backend", this);

    // 3. Cargar el archivo QML
    ui->visorQml->setSource(QUrl("qrc:/interfaz.qml"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::iniciarMotorDesdeC()
{
    qDebug() << "¡QML presionó el botón! Llamando a la librería en C...";
}

// NUEVA IMPLEMENTACIÓN: Toma los valores de QML y los manda al microcontrolador
void MainWindow::configurarHeartbeat(bool habilitado, int periodo)
{
    if (puertoSerie->Comprobar()) {
        // La carga total es 4 bytes: [ID: 1 byte] + [Habilitado: 1 byte] + [Periodo: 2 bytes]
        puertoSerie->AbrirCarga(4);
        puertoSerie->AgregarDato((uint8_t)0xAD); // Comando de configuración
        puertoSerie->AgregarDato((uint8_t)(habilitado ? 1 : 0));
        puertoSerie->AgregarDato((uint16_t)periodo);
        puertoSerie->CerrarCarga();
        puertoSerie->EnviarBufTx();

        qDebug() << "Heartbeat configurado -> Estado:" << habilitado << "| Periodo:" << periodo << "ms";
    } else {
        qDebug() << "Ignorado: El puerto serie no está abierto.";
    }
}

void MainWindow::onRx()
{
    // Leemos los bytes entrantes
    puertoSerie->Recibir();

    // Verificamos si se decodificó un paquete válido
    if (puertoSerie->Comando()) {
        switch (puertoSerie->IDComando()) {
        case 0xAA: // Recepción del latido (Heartbeat)
            // Podrías poner una pequeña animación en QML o imprimir en consola
            qDebug() << "Latido recibido:" << puertoSerie->ObtenerUint16_t(1);
            break;

        case 0xAB: // Comando ALIVE de confirmación
            ui->pushButtonOpenClose->setText("CLOSE");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: green; color: black; font-weight: bold; }");
            qDebug() << "Handshake exitoso. Conexión establecida.";
            break;

        case 0xFF:
            // Otro comando de ejemplo
            break;

        default:
            qDebug() << "Comando desconocido recibido:" << Qt::hex << puertoSerie->IDComando();
            break;
        }
    }
}

void MainWindow::onTimer()
{
    //puertoSerie->Transmitir();
}

void MainWindow::on_pushButtonOpenClose_clicked()
{
    if (puertoSerie->Comprobar()) {
        // --- PROCESO DE DESCONEXIÓN ---

        // 1. Enviamos el mensaje de finalización (0xAC)
        puertoSerie->AbrirCarga(sizeof(uint8_t));
        puertoSerie->AgregarDato((uint8_t)0xAC);
        puertoSerie->CerrarCarga();
        puertoSerie->EnviarBufTx();

        // 2. Esperamos 50ms asíncronos antes de cerrar el puerto
        QTimer::singleShot(50, this, [this]() {
            puertoSerie->CerrarPuerto();
            ui->pushButtonOpenClose->setText("OPEN");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: red; color: white; font-weight: bold; }");
            qDebug() << "Puerto cerrado.";
        });

    } else {
        // --- PROCESO DE CONEXIÓN ---

        if (puertoSerie->AbrirPuerto(ui->comboBoxPuertos->currentText(), QSerialPort::Baud115200, QIODevice::ReadWrite)) {

            // 1. Cambiamos la interfaz a estado de ESPERA
            ui->pushButtonOpenClose->setText("WAIT...");
            ui->pushButtonOpenClose->setStyleSheet("QPushButton { background-color: yellow; color: black; font-weight: bold; }");

            // 2. Enviamos el mensaje de Alive (0xAB) al micro
            puertoSerie->AbrirCarga(sizeof(uint8_t));
            puertoSerie->AgregarDato((uint8_t)0xAB);
            puertoSerie->CerrarCarga();
            puertoSerie->EnviarBufTx();
        } else {
            qDebug() << "Error: No se pudo abrir el puerto.";
        }
    }
}
