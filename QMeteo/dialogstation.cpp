#include "dialogstation.h"
#include "ui_dialogstation.h"

DialogStation::DialogStation(long station, QString remote, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogStation)
{
    this->station = station;
    this->meteo = new QMeteo();
    ui->setupUi(this);

    this->meteo->setRemote(remote);
    ui->cmbTimespan->addItem("3h");
    ui->cmbTimespan->addItem("12h");
    ui->cmbTimespan->addItem("24h");
    ui->cmbTimespan->addItem("48h");
    ui->cmbTimespan->addItem("72h");
    ui->cmbTimespan->addItem("7d");
    ui->cmbTimespan->addItem("30d");
    ui->cmbTimespan->addItem("Custom ...");

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%d %h:%m:%s");
    ui->plot->xAxis->setTicker(timeTicker);
}

DialogStation::~DialogStation()
{
    delete ui;
    delete this->meteo;
}

void DialogStation::on_btnPlot_clicked()
{
    this->plot();
}


void DialogStation::plot() {
    long maxTimestamp = QDateTime::currentMSecsSinceEpoch()/1000L;
    long minTimestamp = maxTimestamp;
    long limit = 1000;

    if(ui->cmbTimespan->currentIndex() == 0)        // 3h
        minTimestamp = maxTimestamp - 3*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 1)       // 12h
        minTimestamp = maxTimestamp - 12*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 2)       // 24h
        minTimestamp = maxTimestamp - 24*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 3)       // 48h
        minTimestamp = maxTimestamp - 48*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 4)       // 72h
        minTimestamp = maxTimestamp - 72*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 5)       // 7d
        minTimestamp = maxTimestamp - 7*24*60*60L;
    else if(ui->cmbTimespan->currentIndex() == 6)       // 30d
        minTimestamp = maxTimestamp - 30*24*60*60L;
    else {
        // XXX: Not yet implemented :-(
        ui->lblStatus->setText("Status: Not yet implemented (Sorry!)");
        return;
    }


    QList<DataPoint> datapoints = this->meteo->query(this->station, minTimestamp, maxTimestamp, limit);
    QVector<double> x,y;
    ui->plot->clearGraphs();

    QCPGraph *graph = ui->plot->addGraph(ui->plot->xAxis, ui->plot->yAxis);
    foreach(const DataPoint &dp, datapoints) {
        x.append(dp.timestamp);
        y.append(dp.t);
    }
    graph->setData(x,y);
    QPen pen = QPen(QColor::fromRgb(0,0, 128, 255));
    pen.setWidth(2);
    graph->setPen(pen);

    graph = ui->plot->addGraph(ui->plot->xAxis, ui->plot->yAxis2);
    x.clear(); y.clear();
    foreach(const DataPoint &dp, datapoints) {
        x.append(dp.timestamp);
        y.append(dp.hum);
    }
    graph->setData(x,y);
    pen = QPen(QColor::fromRgb(0, 128, 0, 255));
    pen.setWidth(2);
    graph->setPen(pen);
    x.clear(); y.clear();

    ui->plot->rescaleAxes(true);
    ui->plot->replot();
    ui->lblStatus->setText("Status: Fetched " + QString::number(datapoints.size()) + " datapoints");
}
