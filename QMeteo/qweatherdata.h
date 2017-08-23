#ifndef QWEATHERDATA_H
#define QWEATHERDATA_H

#include <QWidget>
#include <QMenu>
#include <QDateTime>

namespace Ui {
class QWeatherData;
}

class QWeatherData : public QWidget
{
    Q_OBJECT

private:
    /** Helper call */
    void processReadings(const float value, float &current, float &smoothed, float &average, bool &hasData);

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
    void updateTimestamp();

    void setTemperatureRange(const int min, const int max);
    void setHumidity(const int min, const int max);

    /**
     * @brief addLightning Increase lightning number by one
     * @return current lightning number
     */
    long addLightning();
    void setLightningDisturberDetected();
    void setLightningNoiseDetected();

    long getLightnings() const { return this->lightnings; }

    long stationId() const { return this->_stationId; }
    void stationId(const long id) { this->_stationId = id; }

private slots:
    void on_actionClear_triggered();

    void on_QWeatherData_customContextMenuRequested(const QPoint &pos);

    void on_actionShowSmoothed_triggered();

    void on_actionShowCurrent_triggered();

    void on_actionShowAverage_triggered();

    void on_lblTitle_linkActivated(const QString &link);

signals:
    void onLinkClicked(const QString &link, long station);

private:
    Ui::QWeatherData *ui;

    float temperature;
    float humidity;
    float pressure;
    float light;
    long lightnings = 0;

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
    long _stationId;       // Station id for the links
};

#endif // QWEATHERDATA_H
