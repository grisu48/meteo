#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

#include "receiverthread.h"

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


    void receiver_newData(const long station, QMap<QString, double> values);
    void receiver_error(int socketError, const QString &message);
    void receiver_parseError(QString &message, QString &packet);

private:
    Ui::MainWindow *ui;

    ReceiverThread *receiver = NULL;

    void connectStation(QString remote, int port = 8888);
    void closeConnection(void);

    /** Currently connected remote */
    QString remote;
    /** Currently connected port */
    int port = 0;
};

#endif // MAINWINDOW_H
