#ifndef METEO_WEATHERDATA_H
#define METEO_WEATHERDATA_H

/** Single datapoint */
class WeatherData
{
private:

    long _station;
    float _temperature;
    float _humidity;
    float _pressure;
    float _light;

    long _timestamp;
public:
    WeatherData();
    WeatherData(long station, float temperature, float humidity, float pressure, float light, long timestamp);
    WeatherData(const WeatherData &src);

    long station(void) const { return this->_station; }
    float temperature(void) const { return this->_temperature; }
    float humidity(void) const { return this->_humidity; }
    float pressure(void) const { return this->_pressure; }
    float light(void) const { return this->_light; }
    long timestamp(void) const { return this->_timestamp; }
};

#if 0

/** Station data */
class Station {
private:
    QString _name;
    long _id;
public:
    Station();
    Station(const QString &name, const long id) {
        this->_name = name;
        this->_id = id;
    }
    Station(const long id) {
        this->_id = id;
    }
    Station(const QString &name) {
        this->_name = name;
    }
    virtual ~Station() {}

    QString name(void) const { return this->_name; }
    long id(void) const { return this->_id; }

    void setName(const QString &name) { this->_name = name; }
    void setId(const long id) { this->_id = id; }

};

#endif

#endif // WEATHERDATA_H
