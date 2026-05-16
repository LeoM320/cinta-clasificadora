#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQmlContext>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    // encender_motor_cinta(1);
}
