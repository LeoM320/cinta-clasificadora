#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "UnerSerialQT.h"
#include <QQuickWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // SEÑALES HACIA QML
signals:
    void distanceUpdated(int cm);
    void irStatesUpdated(bool ir0, bool ir1, bool ir2, bool ir3);
    void connectionStatusChanged(bool connected);

public slots:
    Q_INVOKABLE void encenderCinta();
    Q_INVOKABLE void apagarCinta();
    Q_INVOKABLE void setServo(int servoId, int angulo);

    // Nuevos slots para solicitar telemetría
    Q_INVOKABLE void requestDistance();
    Q_INVOKABLE void requestIrStates();

    Q_INVOKABLE void iniciarMotorDesdeC();
    Q_INVOKABLE void configurarHeartbeat(bool habilitado, int periodo);

    void onRx();
    void onTimer();

private slots:
    void on_pushButtonOpenClose_clicked();

private:
    Ui::MainWindow *ui;
    UnerSerialQT *puertoSerie;
};

#endif
