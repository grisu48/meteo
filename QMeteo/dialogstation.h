#ifndef DIALOGSTATION_H
#define DIALOGSTATION_H

#include <QDialog>
#include <QDateTime>

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

private slots:
    void on_btnPlot_clicked();

    void plot_mouseMove(QMouseEvent *event);

private:
    Ui::DialogStation *ui;

    QMeteo *meteo;
    long station;

    QVector<DataPoint> datapoints;


    void plot();
};

#endif // DIALOGSTATION_H
