#ifndef DIALOGSTATION_H
#define DIALOGSTATION_H

#include <QDialog>
#include <QTimer>
#include <QDateTime>

#include "qcustomplot.h"
#include "entities.h"
#include "dbmanager.h"

namespace Ui {
class DialogStation;
}

class DialogStation : public QDialog
{
    Q_OBJECT

public:
    explicit DialogStation(long station, DbManager *db, QWidget *parent = 0);
    ~DialogStation();

private slots:

    void replotGraphs();
    void setAutoReplot(bool enable);
    void setScatters(bool enable);

    void on_rbTime3h_toggled(bool checked);

    void on_rbTime12h_toggled(bool checked);

    void on_rbTime48_toggled(bool checked);

    void on_rbTimeCustom_toggled(bool checked);

    void on_btnSetCustomTime_clicked();

    void on_plPlot_customContextMenuRequested(const QPoint &pos);

private:
    Ui::DialogStation *ui;
    Station station;
    DbManager *db;
    QTimer *timer;


    long timeOffset = 0L;       // Offset to current time in seconds
    long deltaT = 3*60*60L;     // Time offset in seconds to the past

    /** Show plots 0 = Temperature, 1 = Humidity, 2 = Pressure, 3 = Light */
    bool showPlots[4] = { true, true, true, false };

    bool scatters = true;
    int lineWidth = 4;

    QCPGraph *addGraph(QVector<double> &x, QVector<double> &y, QColor color, bool axis2 = false);
};

#endif // DIALOGSTATION_H
