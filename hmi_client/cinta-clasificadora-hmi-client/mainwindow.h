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

signals:
    void distanceUpdated(int cm);
    void irStatesUpdated(bool ir0, bool ir1, bool ir2, bool ir3);
    void connectionStatusChanged(bool connected);
    // NUEVA SEÑAL PARA EL LOG
    void logMessageReceived(QString message);

public slots:
    Q_INVOKABLE void encenderCinta();
    Q_INVOKABLE void apagarCinta();
    Q_INVOKABLE void setServo(int servoId, int angulo);

    void requestDistance();
    void requestIrStates();
    void onRx();

private slots:
    void on_pushButtonOpenClose_clicked();
    void onTimerPolling(); // Nuevo slot para el bucle continuo

private:
    Ui::MainWindow *ui;
    UnerSerialQT *puertoSerie;
    QTimer *timerPolling; // Temporizador para la telemetría
};
#endif
