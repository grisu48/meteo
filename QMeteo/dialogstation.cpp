#include "dialogstation.h"
#include "ui_dialogstation.h"

DialogStation::DialogStation(long station, DbManager *db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogStation)
{
    bool ok;
    this->station = db->station(station, &ok);
    this->db = db;
    this->timer = new QTimer(this);
    ui->setupUi(this);

    this->setWindowTitle("Station " + this->station.name);
    ui->lblTitle->setText(this->station.name);
    if(!ok) {
        ui->lblStatus->setText("Station: Station not found in database");
    }

    ui->plPlot->yAxis2->setVisible(true);

    connect(this->timer, SIGNAL(timeout()), this, SLOT(timerCall()));
    this->timer->setInterval(2000);
    this->timer->start();
    this->timerCall();      // First refresh
}

DialogStation::~DialogStation()
{
    delete this->timer;
    delete ui;
}

static QCPGraph *addGraph(QCustomPlot *plot, QVector<double> &x, QVector<double> &y, QColor color, bool axis2 = false) {
    QCPGraph *graph;
    if(axis2)
        graph = plot->addGraph(plot->xAxis, plot->yAxis2);
    else
        graph = plot->addGraph(plot->xAxis, plot->yAxis);
    graph->setData(x, y);
    QPen pen(color);
    pen.setWidth(4);
    graph->setPen(pen);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color, color, 7));
    return graph;
}

void DialogStation::timerCall() {
    QDateTime today = QDateTime::currentDateTime();
    today.setTime(QTime(0,0,0,0));
    const long lToday = (today.toMSecsSinceEpoch()/1000L);

    const long t_1 = (QDateTime::currentMSecsSinceEpoch()/1000L) + timeOffset;
    const long t_0 = t_1 - this->deltaT;

    QList<DataPoint> points = db->getDatapoints(this->station.id, t_0, t_1, 1000);

    ui->plPlot->clearGraphs();

    QVector<double> timestamps;
    foreach(const DataPoint &dp, points)
        timestamps.push_back(dp.timestamp-lToday);

    QVector<double> y;      // Buffer for y values
    if(this->showPlots[0]) {        // Temperature
        y.clear();
        foreach(const DataPoint &dp, points)
            y.push_back(dp.t);
        addGraph(ui->plPlot, timestamps, y, QColor(0, 0, 128, 255), false);
    }

    if(this->showPlots[1]) {
        y.clear();
        foreach(const DataPoint &dp, points)
            y.push_back(dp.hum);
        addGraph(ui->plPlot, timestamps, y, QColor(0, 128, 0, 255), true);
    }

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->plPlot->xAxis->setTicker(timeTicker);

    ui->plPlot->rescaleAxes(true);
    ui->plPlot->replot();
}

void DialogStation::on_rbTime3h_toggled(bool checked)
{
    if(checked) {
        this->deltaT = 3L*60L*60L;
        this->timerCall();
    }
}

void DialogStation::on_rbTime12h_toggled(bool checked)
{
    if(checked) {
        this->deltaT = 12L*60L*60L;
        this->timerCall();
    }
}

void DialogStation::on_rbTime48_toggled(bool checked)
{
    if(checked) {
        this->deltaT = 48L*60L*60L;
        this->timerCall();
    }
}
