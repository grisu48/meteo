#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>

#include "qmeteo.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionConnect_triggered();

    void on_actionDisconnect_triggered();

    void on_actionQuit_triggered();

    void onDataReceived(const QList<DataPoint> datapoints);

private:
    Ui::MainWindow *ui;

    QMeteo *meteo = NULL;
};

#endif // MAINWINDOW_H
