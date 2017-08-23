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

    ui->gbCustomTime->setVisible(ui->rbTimeCustom->isChecked());
    ui->cutstomT0->setDateTime(QDateTime::currentDateTime());
    ui->txtCustomDeltaT->setText("6");

    this->setWindowTitle("Station " + this->station.name);
    ui->lblTitle->setText(this->station.name);
    if(!ok) {
        ui->lblStatus->setText("Station: Station not found in database");
    }

    ui->plPlot->yAxis2->setVisible(true);

    connect(this->timer, SIGNAL(timeout()), this, SLOT(replotGraphs()));
    this->timer->setInterval(2000);
    this->timer->start();
    this->replotGraphs();      // First refresh
}

DialogStation::~DialogStation()
{
    delete this->timer;
    delete ui;
}

QCPGraph* DialogStation::addGraph(QVector<double> &x, QVector<double> &y, QColor color, bool axis2) {
    QCustomPlot *plot = ui->plPlot;
    QCPGraph *graph;
    if(axis2)
        graph = plot->addGraph(plot->xAxis, plot->yAxis2);
    else
        graph = plot->addGraph(plot->xAxis, plot->yAxis);
    graph->setData(x, y);
    QPen pen(color);
    pen.setWidth(this->lineWidth);
    graph->setPen(pen);
    if(this->scatters)
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color, color, 7));
    return graph;
}

void DialogStation::replotGraphs() {
    QDateTime today = QDateTime::currentDateTime();
    today.setTime(QTime(0,0,0,0));
    const long lToday = (today.toMSecsSinceEpoch()/1000L);

    long t_1;
    if(ui->rbTimeCustom->isChecked()) {
        t_1 = ui->cutstomT0->dateTime().toMSecsSinceEpoch()/1000L;
    } else
        t_1 = (QDateTime::currentMSecsSinceEpoch()/1000L) + timeOffset;
    const long t_0 = t_1 - this->deltaT;
    int limit = 1000;
    if(!ui->txtCustomLimit->text().isEmpty()) {
        bool ok;
        limit = ui->txtCustomLimit->text().toInt(&ok);
        if(!ok) limit = 1000;
        if(limit <= 0) limit = 1000;
    }

    QList<DataPoint> points = db->getDatapoints(this->station.id, t_0, t_1, limit);
    ui->lblStatus->setText(QString::number(points.size()) + " datapoints fetched");

    ui->plPlot->clearGraphs();

    QVector<double> timestamps;
    foreach(const DataPoint &dp, points)
        timestamps.push_back(dp.timestamp-lToday);

    QVector<double> y;      // Buffer for y values
    if(this->showPlots[0]) {        // Temperature
        y.clear();
        foreach(const DataPoint &dp, points)
            y.push_back(dp.t);
        addGraph(timestamps, y, QColor(0, 0, 128, 255), false);
    }

    if(this->showPlots[1]) {
        y.clear();
        foreach(const DataPoint &dp, points)
            y.push_back(dp.hum);
        addGraph(timestamps, y, QColor(0, 128, 0, 255), true);
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
        this->replotGraphs();
    }
}

void DialogStation::on_rbTime12h_toggled(bool checked)
{
    if(checked) {
        this->deltaT = 12L*60L*60L;
        this->replotGraphs();
    }
}

void DialogStation::on_rbTime48_toggled(bool checked)
{
    if(checked) {
        this->deltaT = 48L*60L*60L;
        this->replotGraphs();
    }
}

void DialogStation::on_rbTimeCustom_toggled(bool checked)
{
    ui->gbCustomTime->setVisible(checked);
}

void DialogStation::on_btnSetCustomTime_clicked()
{
    bool ok = false;
    int hours = ui->txtCustomLimit->text().toInt(&ok);
    if(ok) {
        this->deltaT = hours*60L*60L;
    }
}

void DialogStation::setAutoReplot(bool enable) {
    if(enable)
        this->timer->start(2000);
    else
        this->timer->stop();
}

void DialogStation::setScatters(bool enable) {
    this->scatters = enable;
    this->replotGraphs();
}

void DialogStation::on_plPlot_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(ui->plPlot);

    QAction *actionReplot = new QAction("Replot", menu);
    connect(actionReplot, SIGNAL(triggered(bool)), this, SLOT(replotGraphs()));
    QAction *actionAutoPlot = new QAction("Auto-Plot", menu);
    actionAutoPlot->setCheckable(true);
    actionAutoPlot->setChecked(this->timer->isActive());
    connect(actionAutoPlot, SIGNAL(triggered(bool)), this, SLOT(setAutoReplot(bool)));
    QAction *actionScatters = new QAction("Scatters", menu);
    actionScatters->setCheckable(true);
    actionScatters->setChecked(this->scatters);
    connect(actionScatters, SIGNAL(triggered(bool)), this, SLOT(setScatters(bool)));

    menu->addAction(actionReplot);
    menu->addAction(actionAutoPlot);
    menu->addSeparator();
    menu->addAction(actionScatters);

    menu->popup(ui->plPlot->mapToGlobal(pos));
    menu->exec();
}
