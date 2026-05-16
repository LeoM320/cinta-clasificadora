#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickWidget>

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
    // Q_INVOKABLE permite que QML pueda llamar a esta función directamente
    Q_INVOKABLE void iniciarMotorDesdeC();

private:
    Ui::MainWindow *ui;
    QQuickWidget *visorQml; // El widget contenedor
};
#endif // MAINWINDOW_H
