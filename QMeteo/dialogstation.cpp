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
}

DialogStation::~DialogStation()
{
    delete ui;
    delete this->meteo;
}
