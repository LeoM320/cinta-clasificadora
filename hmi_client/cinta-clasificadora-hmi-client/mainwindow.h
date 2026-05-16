#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "UnerSerialQT.h"
#include <QQuickWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    Q_INVOKABLE void iniciarMotorDesdeC();

    // NUEVO: Método para recibir los datos de QML
    Q_INVOKABLE void configurarHeartbeat(bool habilitado, int periodo);

    void onRx();
    void onTimer();

private slots:
    void on_pushButtonOpenClose_clicked();

private:
    Ui::MainWindow *ui;
    UnerSerialQT *puertoSerie;
    //QTimer *timer;
};

#endif // MAINWINDOW_H
