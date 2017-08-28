#ifndef DIALOGSTATION_H
#define DIALOGSTATION_H

#include <QDialog>

#include "qmeteo.h"

namespace Ui {
class DialogStation;
}

class DialogStation : public QDialog
{
    Q_OBJECT

public:
    explicit DialogStation(long station, QString remote, QWidget *parent = 0);
    ~DialogStation();

private:
    Ui::DialogStation *ui;

    QMeteo *meteo;
    long station;
};

#endif // DIALOGSTATION_H
