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

    // Inicializar la interfaz asumiendo que el micro arranca en Modo Sensor
    ui->modo_sensor_btn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; border-radius: 4px; padding: 5px;");

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

    //ui->visorQml->setResizeMode(QQuickWidget::SizeRootObjectToView);
    //ui->visorQml->rootContext()->setContextProperty("backend", this);
    //ui->visorQml->setSource(QUrl("qrc:/interfaz.qml"));
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
    {
        uint16_t dist = puertoSerie->ObtenerUint16_t(1);
        ui->distancia_lbl->setText(QString("Distancia: %1 mm").arg(dist));
        break;
    }

    case 0x85: // ACK_GET_IR_STATES
    {
        uint8_t irPack = puertoSerie->ObtenerUint8_t(1);
        bool ir0 = (irPack & 1);
        bool ir1 = (irPack & 2);
        bool ir2 = (irPack & 4);
        bool ir3 = (irPack & 8);
        QString status = QString("IR0:%1  IR1:%2  IR2:%3  IR3:%4")
                             .arg(ir0 ? "ON" : "OFF")
                             .arg(ir1 ? "ON" : "OFF")
                             .arg(ir2 ? "ON" : "OFF")
                             .arg(ir3 ? "ON" : "OFF");

        ui->irs_lbl->setText(status);
        break;
    }

    case 0x09: // CMD_SEND_LOG
    {
        QByteArray textBytes;
        int i = 1;

        while (true) {
            uint8_t letra = puertoSerie->ObtenerUint8_t(i);
            // Salimos si encontramos fin de cadena o llegamos al límite
            if (letra == '\0' || i > 65) {
                break;
            }
            textBytes.append((char)letra);
            i++;
        }

        QString logStr = QString::fromLatin1(textBytes);

        // 1. Mostrar en consola de desarrollo
        qDebug() << "LOG:" << logStr;

        // 2. Mostrar en tu QPlainTextEdit (asumiendo que se llama log_txt)
        ui->plainTextEdit->appendPlainText(logStr);

        // 3. Opcional: forzar scroll al final para ver siempre lo nuevo
        ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());

        //emit logMessageReceived(logStr);
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

//void MainWindow::on_pushButton_clicked()
void MainWindow::on_ON_btn_clicked()
{//Encender cinta
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x01); // 1 = Encender
    puertoSerie->CerrarCarga();
}

//void MainWindow::on_pushButton_2_clicked()
void MainWindow::on_OFF_btn_clicked()
{//Apagar cinta
    puertoSerie->AbrirCarga(2);
    puertoSerie->AgregarDato((uint8_t)0x06); // CMD_SET_BELT
    puertoSerie->AgregarDato((uint8_t)0x00); // 0 = Apagar
    puertoSerie->CerrarCarga();
}

// --- SERVO 0 ---
void MainWindow::on_sv1_0_btn_clicked()   { setServo(0, 0); }
void MainWindow::on_sv1_90_btn_clicked()  { setServo(0, 90); }
void MainWindow::on_sv1_180_btn_clicked() { setServo(0, 180); }

// --- SERVO 1 ---
void MainWindow::on_sv2_0_btn_clicked()   { setServo(1, 0); }
void MainWindow::on_sv2_90_btn_clicked()  { setServo(1, 90); }
void MainWindow::on_sv2_180_btn_clicked() { setServo(1, 180); }

// --- SERVO 2 ---
void MainWindow::on_sv3_0_btn_clicked()   { setServo(2, 0); }
void MainWindow::on_sv3_90_btn_clicked()  { setServo(2, 90); }
void MainWindow::on_sv3_180_btn_clicked() { setServo(2, 180); }

// --- ESTACIÓN 1 ---
void MainWindow::on_coord_est1_btn_clicked() {
    uint32_t valor = ui->coord_est1_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetCoordenadaEstacion1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv0max_btn_clicked() {
    uint32_t valor = ui->set_sv0max_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMax0);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv0min_btn_clicked() {
    uint32_t valor = ui->set_sv0min_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMin0);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_desp_sv0_btn_clicked() {
    uint32_t valor = ui->desp_sv0_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsDesplegar0);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_esp_sv0_btn_clicked() {
    uint32_t valor = ui->esp_sv0_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsEsperar0);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_ret_sv0_btn_clicked() {
    uint32_t valor = ui->ret_sv0_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsRetraer0);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// --- ESTACIÓN 2 ---
void MainWindow::on_coord_est2_btn_clicked() {
    uint32_t valor = ui->coord_est2_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetCoordenadaEstacion2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv1max_btn_clicked() {
    uint32_t valor = ui->set_sv1max_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMax1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv1min_btn_clicked() {
    uint32_t valor = ui->set_sv1min_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMin1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_desp_sv1_btn_clicked() {
    uint32_t valor = ui->desp_sv1_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsDesplegar1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_esp_sv1_btn_clicked() {
    uint32_t valor = ui->esp_sv1_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsEsperar1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_ret_sv1_btn_clicked() {
    uint32_t valor = ui->ret_sv1_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsRetraer1);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// --- ESTACIÓN 3 ---S
void MainWindow::on_coord_est3_btn_clicked() {
    uint32_t valor = ui->coord_est3_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetCoordenadaEstacion3);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv2max_btn_clicked() {
    uint32_t valor = ui->set_sv2max_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMax2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_sv2min_btn_clicked() {
    uint32_t valor = ui->set_sv2min_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetServoMin2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_desp_sv2_btn_clicked() {
    uint32_t valor = ui->desp_sv2_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsDesplegar2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_esp_sv2_btn_clicked() {
    uint32_t valor = ui->esp_sv2_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsEsperar2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_ret_sv2_btn_clicked() {
    uint32_t valor = ui->ret_sv2_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetMsRetraer2);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_dist_hcsr_btn_clicked() {
    uint32_t valor = ui->dist_hcsr_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetDistanciaBase);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_disparosMax_btn_clicked() {
    uint32_t valor = ui->disparosMax_spinBox->value();
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);
    puertoSerie->AgregarDato((uint8_t)eSetDisparosMax);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_modo_sensor_btn_clicked()
{
    if (!puertoSerie->Comprobar()) return;

    // 1. Envío del comando al microcontrolador
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25); // CMD_SET_VARIABLE
    puertoSerie->AgregarDato((uint8_t)eSetCiego);
    puertoSerie->AgregarDato((uint32_t)0);  // 0 = Modo Sensor (Lazo Cerrado)
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();

    // 2. Feedback Visual (HMI)
    // Pintamos el botón Sensor de verde y reseteamos el Ciego a su estado gris
    ui->modo_sensor_btn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; border-radius: 4px; padding: 5px;");
    ui->modo_ciego_btn->setStyleSheet("");  // Un string vacío restaura el tema del SO

    qDebug() << "HMI: Modo cambiado a SENSOR";
}

void MainWindow::on_modo_ciego_btn_clicked()
{
    if (!puertoSerie->Comprobar()) return;

    // 1. Envío del comando al microcontrolador
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25); // CMD_SET_VARIABLE
    puertoSerie->AgregarDato((uint8_t)eSetCiego);
    puertoSerie->AgregarDato((uint32_t)1);  // 1 = Modo Ciego
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();

    // 2. Feedback Visual (HMI)
    // Pintamos el botón Ciego de verde y reseteamos el Sensor
    ui->modo_ciego_btn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; border-radius: 4px; padding: 5px;");
    ui->modo_sensor_btn->setStyleSheet("");

    qDebug() << "HMI: Modo cambiado a CIEGO";
}

// --- ENRUTAMIENTO CAJA A ---
void MainWindow::on_dest_cajaA_btn_clicked() {
    uint32_t valor = ui->dest_cajaA_spinBox->value();
    
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);          // CMD_SET_VARIABLE
    puertoSerie->AgregarDato((uint8_t)eSetDestinoA);
    puertoSerie->AgregarDato((uint32_t)valor);        // Estación (0, 1, 2, 3)
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// --- ENRUTAMIENTO CAJA B ---
void MainWindow::on_dest_cajaB_btn_clicked() {
    uint32_t valor = ui->dest_cajaB_spinBox->value();
    
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);          // CMD_SET_VARIABLE
    puertoSerie->AgregarDato((uint8_t)eSetDestinoB);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

// --- ENRUTAMIENTO CAJA C ---
void MainWindow::on_dest_cajaC_btn_clicked() {
    uint32_t valor = ui->dest_cajaC_spinBox->value();
    
    puertoSerie->AbrirCarga(6);
    puertoSerie->AgregarDato((uint8_t)0x25);          // CMD_SET_VARIABLE
    puertoSerie->AgregarDato((uint8_t)eSetDestinoC);
    puertoSerie->AgregarDato((uint32_t)valor);
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_check_config_btn_clicked() {
    if (!puertoSerie->Comprobar()) return;
    
    puertoSerie->AbrirCarga(1);
    puertoSerie->AgregarDato((uint8_t)0x26);         // CMD_GET_VARIABLE
    puertoSerie->CerrarCarga();
    puertoSerie->EnviarBufTx();
}

void MainWindow::on_set_config_btn_clicked() {
    if (!puertoSerie->Comprobar()) return;

    int delayAcumulado = 0;
    const int stepMs = 30; // 30 ms de respiro entre tramas (100% seguro para AVR)

    // Lambda asíncrona: No envía el dato ahora, sino que lo AGENDARÁ en el Event Loop
    auto agendarParametro = [this, &delayAcumulado, stepMs](uint8_t idParam, uint32_t valor) {

        // QTimer::singleShot ejecuta el código en el futuro sin bloquear
        QTimer::singleShot(delayAcumulado, this, [this, idParam, valor]() {
            if (!puertoSerie->Comprobar()) return; // Aborta si se desconectó el cable

            puertoSerie->AbrirCarga(6);
            puertoSerie->AgregarDato((uint8_t)0x25); // CMD_SET_VARIABLE
            puertoSerie->AgregarDato(idParam);
            puertoSerie->AgregarDato(valor);
            puertoSerie->CerrarCarga();
            puertoSerie->EnviarBufTx();
        });

        // Incrementamos el offset temporal para el próximo parámetro
        delayAcumulado += stepMs;
    };

    // ==========================================
    // AGENDAMIENTO MASIVO (Batch Provisioning)
    // ==========================================

    // --- GLOBALES ---
    agendarParametro(eSetDistanciaBase, ui->dist_hcsr_spinBox->value());
    agendarParametro(eSetDisparosMax, ui->disparosMax_spinBox->value());

    // --- CLASIFICACIÓN Y ENRUTAMIENTO ---
    // (Asumiendo que tenés estos spinboxes o cambialos por los tuyos)
    agendarParametro(eSetDestinoA, ui->dest_cajaA_spinBox->value());
    agendarParametro(eSetDestinoB, ui->dest_cajaB_spinBox->value());
    agendarParametro(eSetDestinoC, ui->dest_cajaC_spinBox->value());

    // --- ESTACIÓN 1 ---
    agendarParametro(eSetCoordenadaEstacion1, ui->coord_est1_spinBox->value());
    agendarParametro(eSetMsEsperar0, ui->esp_sv0_spinBox->value());
    agendarParametro(eSetMsDesplegar0, ui->desp_sv0_spinBox->value());
    agendarParametro(eSetMsRetraer0, ui->ret_sv0_spinBox->value());
    agendarParametro(eSetServoMax0, ui->set_sv0max_spinBox->value());
    agendarParametro(eSetServoMin0, ui->set_sv0min_spinBox->value());

    // --- ESTACIÓN 2 ---
    agendarParametro(eSetCoordenadaEstacion2, ui->coord_est2_spinBox->value());
    agendarParametro(eSetMsEsperar1, ui->esp_sv1_spinBox->value());
    agendarParametro(eSetMsDesplegar1, ui->desp_sv1_spinBox->value());
    agendarParametro(eSetMsRetraer1, ui->ret_sv1_spinBox->value());
    agendarParametro(eSetServoMax1, ui->set_sv1max_spinBox->value());
    agendarParametro(eSetServoMin1, ui->set_sv1min_spinBox->value());

    // --- ESTACIÓN 3 ---
    agendarParametro(eSetCoordenadaEstacion3, ui->coord_est3_spinBox->value());
    agendarParametro(eSetMsEsperar2, ui->esp_sv2_spinBox->value());
    agendarParametro(eSetMsDesplegar2, ui->desp_sv2_spinBox->value());
    agendarParametro(eSetMsRetraer2, ui->ret_sv2_spinBox->value());
    agendarParametro(eSetServoMax2, ui->set_sv2max_spinBox->value());
    agendarParametro(eSetServoMin2, ui->set_sv2min_spinBox->value());

    // Opcional: Logueamos en la consola para confirmar que se disparó la ráfaga
    qDebug() << "Batch programado. Tiempo estimado de transferencia:" << delayAcumulado << "ms";
}
