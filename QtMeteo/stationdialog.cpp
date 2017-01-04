#include "stationdialog.h"
#include "ui_stationdialog.h"

StationDialog::StationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StationDialog)
{
    ui->setupUi(this);

    this->station = 0;
    this->timer = new QTimer(this);
    connect(this->timer, SIGNAL(timeout()), this, SLOT(timerCall()));
    this->timer->start(1000);
}

StationDialog::~StationDialog()
{
    timer->stop();
    delete timer;
    delete ui;
}

void StationDialog::setStation(long station) {
    this->station = station;

    switch(station) {
    case S_ID_FLEX :
        this->setWindowTitle("Station overview - Flex' Room");
        this->timerCall();
        break;
    case S_ID_LIVING :
        this->setWindowTitle("Station overview - Living room");
        this->timerCall();
        break;
    case S_ID_OUTDOOR :
        this->setWindowTitle("Station overview - Outdoor");
        this->timerCall();
        break;
    }
}

static double min(const double x, const double y) { return (x<y?x:y); }
static double max(const double x, const double y) { return (x>y?x:y); }

void StationDialog::showOutdoor() {
    QList<st_outdoor_t> data = Stations::instance()->getOutdoor();
    if(data.size() == 0) return;
    st_outdoor_t &current = data[data.size()-1];

    ui->txtTemperature->setText(QString::number(current.t, 'f', 2) + " deg C");
    ui->txtHumidity->setText(QString::number(current.h, 'f', 2) + " % rel");
    ui->txtPressure->setText(QString::number(current.p, 'f', 2) + " hPa");
    ui->pltStation->clearGraphs();

    int type = 0;
    if(ui->rbTemperature->isChecked())
        type = 1;
    else if(ui->rbHumidity->isChecked())
        type = 2;
    else if(ui->rbPressure->isChecked())
        type = 3;
    else
        return;

    const long t_min = data[0].timestamp;
    QVector<double> times;
    QVector<double> tt;
    bool first = true;
    double v_min, v_max;
    for(auto it = data.begin(); it != data.end(); it++) {
        const long time = (it->timestamp - t_min) / 1000L;

        times.push_back((double)time);
        double value;
        if(type == 1)
            value = it->t;
        else if(type == 2)
            value = it->h;
        else if(type == 3)
            value = it->p;

        tt.push_back(value);

        if(first) {
            v_min = value;
            v_max = value;
        } else {
            v_min = min(v_min, value);
            v_max = max(v_max, value);
        }
    }

    QCPGraph *gpTemperature = ui->pltStation->addGraph();

    gpTemperature->setData(times, tt);

    if(type == 1) gpTemperature->setPen(QPen(QColor(0,0,200)));
    else if(type == 2) gpTemperature->setPen(QPen(QColor(0,2000,0)));
    else if(type == 3) gpTemperature->setPen(QPen(QColor(128,128,128)));

    if(v_min > 0) v_min *= 0.9;
    else v_min *= 1.1;
    if(v_max > 0) v_max *= 1.1;
    else v_max *= 0.9;

    ui->pltStation->rescaleAxes();
    ui->pltStation->yAxis->setRange(v_min, v_max);
    ui->pltStation->replot();
}

void StationDialog::showIndoor() {
    QList<st_livin_t> data = Stations::instance()->getLivingRoom();
    if(data.size() == 0) return;
    st_livin_t &current = data[data.size()-1];

    ui->txtTemperature->setText(QString::number(current.t, 'f', 2) + " deg C");
    ui->txtHumidity->setText(QString::number(current.h, 'f', 2) + " % rel");
    ui->txtPressure->setText(" -- no readings -- ");
    ui->pltStation->clearGraphs();

    int type = 0;
    if(ui->rbTemperature->isChecked())
        type = 1;
    else if(ui->rbHumidity->isChecked())
        type = 2;
    //else if(ui->rbPressure->isChecked())
    //    return;
    else
        return;

    const long t_min = data[0].timestamp;
    QVector<double> times;
    QVector<double> tt;
    bool first = true;
    double v_min, v_max;
    for(auto it = data.begin(); it != data.end(); it++) {
        const long time = (it->timestamp - t_min) / 1000L;

        times.push_back((double)time);
        double value;
        if(type == 1)
            value = it->t;
        else if(type == 2)
            value = it->h;

        tt.push_back(value);

        if(first) {
            v_min = value;
            v_max = value;
        } else {
            v_min = min(v_min, value);
            v_max = max(v_max, value);
        }
    }

    QCPGraph *gpTemperature = ui->pltStation->addGraph();

    gpTemperature->setData(times, tt);

    if(type == 1) gpTemperature->setPen(QPen(QColor(0,0,200)));
    else if(type == 2) gpTemperature->setPen(QPen(QColor(0,2000,0)));
    else if(type == 3) gpTemperature->setPen(QPen(QColor(128,128,128)));

    if(v_min > 0) v_min *= 0.9;
    else v_min *= 1.1;
    if(v_max > 0) v_max *= 1.1;
    else v_max *= 0.9;

    ui->pltStation->rescaleAxes();
    ui->pltStation->yAxis->setRange(v_min, v_max);
    ui->pltStation->replot();
}

void StationDialog::showFlex() {
    QList<st_flex_t> data = Stations::instance()->getRoomFlex();
    if(data.size() == 0) return;
    st_flex_t &current = data[data.size()-1];

    ui->txtTemperature->setText(QString::number(current.t, 'f', 2) + " deg C");
    ui->txtHumidity->setText(QString::number(current.h, 'f', 2) + " % rel");
    ui->txtPressure->setText(" -- no readings -- ");
    ui->pltStation->clearGraphs();

    int type = 0;
    if(ui->rbTemperature->isChecked())
        type = 1;
    else if(ui->rbHumidity->isChecked())
        type = 2;
    //else if(ui->rbPressure->isChecked())
    //    return;
    else
        return;

    const long t_min = data[0].timestamp;
    QVector<double> times;
    QVector<double> tt;
    bool first = true;
    double v_min, v_max;
    for(auto it = data.begin(); it != data.end(); it++) {
        const long time = (it->timestamp - t_min) / 1000L;

        times.push_back((double)time);
        double value;
        if(type == 1)
            value = it->t;
        else if(type == 2)
            value = it->h;

        tt.push_back(value);

        if(first) {
            v_min = value;
            v_max = value;
        } else {
            v_min = min(v_min, value);
            v_max = max(v_max, value);
        }
    }

    QCPGraph *gpTemperature = ui->pltStation->addGraph();

    gpTemperature->setData(times, tt);

    if(type == 1) gpTemperature->setPen(QPen(QColor(0,0,200)));
    else if(type == 2) gpTemperature->setPen(QPen(QColor(0,2000,0)));
    else if(type == 3) gpTemperature->setPen(QPen(QColor(128,128,128)));

    if(v_min > 0) v_min *= 0.9;
    else v_min *= 1.1;
    if(v_max > 0) v_max *= 1.1;
    else v_max *= 0.9;

    ui->pltStation->rescaleAxes();
    ui->pltStation->yAxis->setRange(v_min, v_max);
    ui->pltStation->replot();

}

void StationDialog::timerCall() {

    switch(this->station) {
    case S_ID_FLEX :
        showFlex();
        break;
    case S_ID_LIVING :
        showIndoor();
        break;
    case S_ID_OUTDOOR :
    showOutdoor();
        break;
    }
}
