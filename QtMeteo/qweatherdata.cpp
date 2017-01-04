#include "qweatherdata.h"
#include "ui_qweatherdata.h"

QWeatherData::QWeatherData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QWeatherData)
{
    ui->setupUi(this);
    this->temperature = 0;
    this->humidity = 0;
    this->light = 0;
    this->pressure = 0;
}

QWeatherData::~QWeatherData()
{
    delete ui;
}

void QWeatherData::setTemperature(const float value) {
    this->temperature = value;
    ui->txtTemperature->setText(QString::number(value, 'f', 2) + " deg C");
}

void QWeatherData::setHumidity(const float value) {
    this->humidity = value;
    ui->txtHumidity->setText(QString::number(value, 'f', 2) + " % rel");
}

void QWeatherData::setPressure(const float value) {
    this->pressure = value;
    ui->txtPressure->setText(QString::number(value/100.0F, 'f', 2) + " mBar");
}

void QWeatherData::setLight(const float value) {
    this->light = value;
    ui->txtLight->setText(QString::number((int) value) + "/255");
}

void QWeatherData::setStatus(QString status) {
    ui->lblStatus->setText(status);
}


void QWeatherData::setName(QString name) {
    this->name = name;
    ui->lblTitle->setText("<a href=\"link\">" + name + "</a>");
}
