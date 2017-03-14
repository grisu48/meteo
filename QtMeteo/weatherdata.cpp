#include "weatherdata.h"

WeatherData::WeatherData()
{
}


WeatherData::WeatherData(long station, float temperature, float humidity, float pressure, float light, long timestamp) {
    this->_station = station;
    this->_temperature = temperature;
    this->_humidity = humidity;
    this->_pressure = pressure;
    this->_light = light;
    this->_timestamp = timestamp;
}

WeatherData::WeatherData(const WeatherData &src) {
    this->_station = src._station;
    this->_temperature = src._temperature;
    this->_humidity = src._humidity;
    this->_pressure = src._pressure;
    this->_light = src._light;
    this->_timestamp = src._timestamp;
}
