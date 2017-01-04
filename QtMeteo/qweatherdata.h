#ifndef QWEATHERDATA_H
#define QWEATHERDATA_H

#include <QWidget>

namespace Ui {
class QWeatherData;
}

class QWeatherData : public QWidget
{
    Q_OBJECT

public:
    explicit QWeatherData(QWidget *parent = 0);
    ~QWeatherData();

    void setTemperature(const float value);
    void setHumidity(const float value);
    void setPressure(const float value);
    void setLight(const float value);
    void setStatus(QString status);

    void setName(QString name);

private:
    Ui::QWeatherData *ui;

    float temperature;
    float humidity;
    float pressure;
    float light;

    QString name;
};

#endif // QWEATHERDATA_H
