#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QQuickWidget>
#include "unerSerialQT.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void encenderCinta();
    void apagarCinta();
    void setServo(int servoId, int angulo);

signals:
    void connectionStatusChanged(bool connected);
    void distanceUpdated(int distance);
    void irStatesUpdated(bool ir0, bool ir1, bool ir2, bool ir3);
    void logMessageReceived(const QString &message);

private slots:
    void onTimerPolling();
    void onRx(uint8_t cmdId);

private:
    void requestDistance();
    void requestIrStates();

    Ui::MainWindow *ui;
    UnerSerialQT *puertoSerie;
    QTimer *timerPolling;
};

#endif // MAINWINDOW_H
