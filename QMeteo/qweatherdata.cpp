#include "qweatherdata.h"
#include "ui_qweatherdata.h"

#include <math.h>

#define ALPHA_SMOOTH 0.85
#define ALPHA_AVG 0.99

QWeatherData::QWeatherData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QWeatherData)
{
    ui->setupUi(this);
    ui->actionShowCurrent->setChecked(true);
    this->clear();
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->prgHumidity->setStyleSheet("QProgressBar::chunk { background-color: #1da33e; width: 1px; } QProgressBar { border: 2px solid grey; border-radius: 0px; text-align: center; }");
    ui->prgTemperature->setStyleSheet("QProgressBar::chunk { background-color: #4286f4; width: 1px; } QProgressBar { border: 2px solid grey; border-radius: 0px; text-align: center; }");
}

QWeatherData::~QWeatherData()
{
    delete ui;
}


void QWeatherData::setTemperatureRange(const int min, const int max) {
    ui->prgTemperature->setRange(min, max);
}

void QWeatherData::setHumidity(const int min, const int max) {
    ui->prgHumidity->setRange(min, max);
}

void QWeatherData::processReadings(const float value, float &current, float &smoothed, float &average, bool &hasData) {
    if(hasData) {
        current = value;
        smoothed = ALPHA_SMOOTH * smoothed + (1.0F - ALPHA_SMOOTH) * value;
        average = ALPHA_AVG * average + (1.0F - ALPHA_AVG) * value;
    } else {
        current = value;
        smoothed = value;
        average = value;
        hasData = true;
    }
}

void QWeatherData::setTemperature(const float value) {
    processReadings(value, this->temperature, this->smth_temperature, this->avg_temperature, this->hasTemperature);
    this->refreshTemperature();
}

void QWeatherData::setHumidity(const float value) {
    processReadings(value, this->humidity, this->smth_humidity, this->avg_humidity, this->hasHumidity);
    this->refreshHumidity();
}

void QWeatherData::setPressure(const float value) {
    processReadings(value, this->pressure, this->smth_pressure, this->avg_pressure, this->hasPressure);
    this->refreshPressure();
}

void QWeatherData::setLight(const float value) {
    this->light = value;
    ui->txtLight->setText(QString::number((int) value) + "/255");
}


void QWeatherData::refreshTemperature(void) {
    if(!hasTemperature) {
        ui->txtTemperature->setText("No temp. data");
    } else {
        float reading = this->temperature;
        if(ui->actionShowSmoothed->isChecked())
            reading = this->smth_temperature;
        else if(ui->actionShowAverage->isChecked())
            reading = this->avg_temperature;
        ui->txtTemperature->setText(QString::number(reading, 'f', 2) + " deg C");
        ui->prgTemperature->setValue((int)reading);
        this->update_dewpoint(this->temperature, this->humidity);
    }
}

void QWeatherData::refreshHumidity(void) {
    if(!hasHumidity) {
        ui->txtHumidity->setText("No humidity data");
    } else {
        float reading = this->humidity;
        if(ui->actionShowSmoothed->isChecked())
            reading = this->smth_humidity;
        else if(ui->actionShowAverage->isChecked())
            reading = this->avg_humidity;
        ui->txtHumidity->setText(QString::number(reading, 'f', 2) + " % rel");
        ui->prgHumidity->setValue((int)reading);
        this->update_dewpoint(this->temperature, this->humidity);
    }
}

void QWeatherData::refreshPressure(void) {
    if(!hasPressure) {
        ui->txtPressure->setText("No pressure data");
    } else {
        float reading = this->pressure;
        if(ui->actionShowSmoothed->isChecked())
            reading = this->smth_pressure;
        else if(ui->actionShowAverage->isChecked())
            reading = this->avg_pressure;
        ui->txtPressure->setText(QString::number(reading/100.0F, 'f', 2) + " mBar");
    }
}

void QWeatherData::refresh(void) {
    refreshTemperature();
    refreshHumidity();
    refreshPressure();
}

void QWeatherData::setStatus(QString status) {
    ui->lblStatus->setText(status);
}


void QWeatherData::setName(QString name) {
    this->name = name;
    ui->lblTitle->setText("<a href=\"link\">" + name + "</a>");
}

void QWeatherData::clear(void) {
    this->temperature = 0;
    this->humidity = 0;
    this->light = 0;
    this->pressure = 0;
    this->avg_temperature = 0.0F;
    this->avg_humidity = 0.0F;
    this->avg_pressure = 0.0F;
    this->smth_temperature = 0.0F;
    this->smth_humidity = 0.0F;
    this->smth_pressure = 0.0F;
    this->hasTemperature = false;
    this->hasHumidity = false;
    this->hasPressure = false;
}

void QWeatherData::on_actionClear_triggered()
{
    this->clear();
    this->refresh();
}

void QWeatherData::on_actionShowSmoothed_triggered()
{
    ui->actionShowAverage->setChecked(false);
    ui->actionShowCurrent->setChecked(false);
    this->refresh();
}

void QWeatherData::on_actionShowCurrent_triggered()
{
    ui->actionShowAverage->setChecked(false);
    ui->actionShowSmoothed->setChecked(false);
    this->refresh();
}

void QWeatherData::on_actionShowAverage_triggered()
{
    ui->actionShowCurrent->setChecked(false);
    ui->actionShowSmoothed->setChecked(false);
    this->refresh();
}


void QWeatherData::on_QWeatherData_customContextMenuRequested(const QPoint &pos)
{
    // Create context menu
    QMenu *menu = new QMenu();
    menu->addAction(ui->actionShowCurrent);
    menu->addAction(ui->actionShowSmoothed);
    menu->addAction(ui->actionShowAverage);
    menu->addSeparator();
    menu->addAction(ui->actionClear);
    menu->popup(this->mapToGlobal(pos));
}

void QWeatherData::on_lblTitle_linkActivated(const QString &link)
{
    emit onLinkClicked(link, this->_stationId);
}


float dew_point(const float t, const float rh) {
    // Values for the Arden Buck equation
    //const float a = 6.1121F;
    const float b = 18.678F;
    const float c = 257.14F;
    const float d = 234.5F;


    const float g_m = ::logf((rh/100.0F) * ::expf((b-t/d)*(t/(c+t))));
    return (c * g_m)/(b-g_m);

}

void QWeatherData::update_dewpoint(const float t, const float rh) {
    float dp = dew_point(t,rh);

    QString feelsLike = "";

    if (dp > 26) feelsLike = "Severely health danger (High humidity)";
    else if(dp > 24) feelsLike = "Oppressive, extremely uncomfortable";
    else if(dp > 21) feelsLike = "Very humid";
    else if(dp > 18) feelsLike = "Uncomfortable";
    else if(dp > 16) feelsLike = "Ok but humid";
    else if(dp > 13) feelsLike = "Comfortable";
    else if(dp > 10) feelsLike = "Very comfortable";
    else if(dp > 5) feelsLike = "A bit dry";
    else feelsLike = "Dry";

    ui->txtDewPoint->setText("(DP: " + QString::number(dp, 'f', 1) + " °C) " + feelsLike);
}
