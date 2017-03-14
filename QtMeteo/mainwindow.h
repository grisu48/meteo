#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QDir>


#include "qweatherdata.h"
#include "stations.h"

#include "receiver.h"
#include "configfile.h"

#define DEFAULT_PORT 8888

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
    void on_actionQuit_triggered();
    void on_actionConnect_triggered();
    void on_actionClose_triggered();


    void receiver_newData(const WeatherData &data);
    void receiver_error(int socketError, const QString &message);
    void receiver_parseError(QString &message, QString &packet);


    void on_actionListen_triggered();

    void on_actionClear_triggered();

private:
    Ui::MainWindow *ui;

    Receiver receiver;
    ConfigFile *config;

    void connectTcp(QString remote, int port = 8888);
    void listenUdp(const int port = 5232);
    void closeConnection(void);

    /** Currently connected remote */
    QString remote;
    /** Currently connected port */
    int port = 0;

    /** Weather widgets*/
    QMap<long, QWeatherData*> w_widgets;
};

#endif // MAINWINDOW_H
