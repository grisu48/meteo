#ifndef STATIONDIALOG_H
#define STATIONDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QList>
#include <QVector>

#include "station.h"

namespace Ui {
class StationDialog;
}

class StationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StationDialog(QWidget *parent = 0);
    ~StationDialog();

    void setStation(long station);

public slots:

    void timerCall();

private:
    Ui::StationDialog *ui;

    /** ID of the station this belongs to */
    long station = 0;

    QTimer *timer;

    void showOutdoor();
    void showIndoor();
    void showFlex();
};

#endif // STATIONDIALOG_H
