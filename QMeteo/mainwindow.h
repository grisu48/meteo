#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QInputDialog>

#include "qmeteo.h"
#include "qweatherdata.h"
#include "dialogstation.h"
#include "config.h"

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
    void onLightningsReceived(const QList<Lightning> lightnings);

    void onStationClicked(QString link, const long station);

    void on_btnFetchLightningToday_clicked();

private:
    Ui::MainWindow *ui;

    QString remoteHost = "localhost:8900";
    QMeteo *meteo = NULL;

    QMap<long, QWeatherData*> stations;
    QList<Lightning> lightnings;

    QWeatherData *station(const long id);
    void clear();
};

#endif // MAINWINDOW_H
