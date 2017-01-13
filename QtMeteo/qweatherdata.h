#ifndef QWEATHERDATA_H
#define QWEATHERDATA_H

#include <QWidget>
#include <QMenu>

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

    /** Clear all current readings */
    void clear(void);

    /** Refresh display */
    void refresh(void);

    void refreshTemperature(void);
    void refreshHumidity(void);
    void refreshPressure(void);

    void setTemperatureRange(const int min, const int max);
    void setHumidity(const int min, const int max);

private slots:
    void on_actionClear_triggered();

    void on_QWeatherData_customContextMenuRequested(const QPoint &pos);

    void on_actionShowSmoothed_triggered();

    void on_actionShowCurrent_triggered();

    void on_actionShowAverage_triggered();

signals:
    void onLinkClicked(void);

private:
    Ui::QWeatherData *ui;

    float temperature;
    float humidity;
    float pressure;
    float light;

    // Averaged values
    float avg_temperature;
    float avg_humidity;
    float avg_pressure;

    // Current readings, smoothed
    float smth_temperature;
    float smth_humidity;
    float smth_pressure;

    bool hasTemperature = false;
    bool hasHumidity = false;
    bool hasPressure = false;

    QString name;
};

#endif // QWEATHERDATA_H
