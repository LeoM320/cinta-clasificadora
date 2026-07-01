#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QQuickWidget>
#include "unerSerialQT.h"
#include <QScrollBar>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef enum {
    eSetServoMin0=0,
    eSetServoMax0,
    eSetServoMin1,
    eSetServoMax1,
    eSetServoMin2,
    eSetServoMax2,
    eSetCiego,
    eSetDelta,
    eSetCoordenadaEstacion1,
    eSetCoordenadaEstacion2,
    eSetCoordenadaEstacion3,
    eSetMsDesplegar0,
    eSetMsRetraer0,
    eSetMsEsperar0,
    eSetMsDesplegar1,
    eSetMsRetraer1,
    eSetMsEsperar1,
    eSetMsDesplegar2,
    eSetMsRetraer2,
    eSetMsEsperar2,
    eSetDistanciaBase,
    eSetDisparosMax,
    eSetHMinA,
    eSetHMaxA,
    eSetHMinB,
    eSetHMaxB,
    eSetHMinC,
    eSetHMaxC,
    eSetDestinoA,
    eSetDestinoB,
    eSetDestinoC
}_eSetVariables;

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

    void on_ON_btn_clicked();

    void on_OFF_btn_clicked();

    void on_sv1_0_btn_clicked();

    void on_sv1_90_btn_clicked();

    void on_sv1_180_btn_clicked();

    void on_sv2_0_btn_clicked();

    void on_sv2_90_btn_clicked();

    void on_sv2_180_btn_clicked();

    void on_sv3_0_btn_clicked();

    void on_sv3_90_btn_clicked();

    void on_sv3_180_btn_clicked();

    void on_coord_est1_btn_clicked();

    void on_set_sv0max_btn_clicked();

    void on_set_sv0min_btn_clicked();

    void on_desp_sv0_btn_clicked();

    void on_esp_sv0_btn_clicked();

    void on_ret_sv0_btn_clicked();

    void on_coord_est2_btn_clicked();

    void on_set_sv1max_btn_clicked();

    void on_set_sv1min_btn_clicked();

    void on_desp_sv1_btn_clicked();

    void on_esp_sv1_btn_clicked();

    void on_ret_sv1_btn_clicked();

    void on_coord_est3_btn_clicked();

    void on_set_sv2max_btn_clicked();

    void on_set_sv2min_btn_clicked();

    void on_desp_sv2_btn_clicked();

    void on_esp_sv2_btn_clicked();

    void on_ret_sv2_btn_clicked();

    void on_dist_hcsr_btn_clicked();

    void on_disparosMax_btn_clicked();

    void on_modo_sensor_btn_clicked();

    void on_modo_ciego_btn_clicked();

    void on_dest_cajaA_btn_clicked();

    void on_dest_cajaB_btn_clicked();

    void on_dest_cajaC_btn_clicked();

    void on_chk_conf_btn_clicked();

    void on_set_conf_btn_clicked();

private:
    void requestDistance();
    void requestIrStates();

    Ui::MainWindow *ui;
    UnerSerialQT *puertoSerie;
    QTimer *timerPolling;
};

#endif // MAINWINDOW_H
