#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

#include "receiverthread.h"

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


    void receiver_newData(const long station, QMap<QString, double> values);
    void receiver_error(int socketError, const QString &message);
    void receiver_parseError(QString &message, QString &packet);
    void on_actionClose_triggered();

private:
    Ui::MainWindow *ui;

    ReceiverThread *receiver = NULL;

    void connectStation(QString remote, int port = 8888);
    void closeConnection(void);
};

#endif // MAINWINDOW_H
