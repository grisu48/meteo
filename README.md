# meteo

[![Build Status](https://travis-ci.org/grisu48/meteo.svg?branch=master)](https://travis-ci.org/grisu48/meteo)

Lightweight environmental monitoring solution

This project aims to provide a centralized environmental and room monitoring system for different sensors using mqtt. Data storage is supported via a `meteo-influxdb` gateway.

# Nodes/Sensors

Current `meteo` sensors are [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview) based nodes with a [BME280](https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/) sensor, but in principal every node that is able to push data via `MQTT` can be attached to the server.

See the [EPS32](Sensors/ESP32) folder in Sensors for the current supported `node`

## MQTT packets

MQTT is considered as trusted network in `meteod`. New station will be automatically added. If you want to have better access control, please use http and disable `MQTT` in your config file.

Every node that publishes `MQTT` packets in the given format is accepted.

    # Node ID: 1, replace in topic and payload
    
    ## meteo packets
    TOPIC:    meteo/1
    PAYLOAD:  {"id":1,"name":"Node","t":23.36,"hum":44.56,"p":99720.89}
    
    ## lightning packet
    TOPIC:    meteo/lightning/1
    PAYLOAD:  {"node":1,"timestamp":0,"distance":12.1}
    # if the timestamp is 0, the server replaces it with it's current time

# meteo-influx-gateway

The provided `meteo-influx-gateway` is a program to collect meteo data points from mqtt and push them to a influxdb database. This gateway is written in go and needs to be build

    go build ./...

The gateway is configured via a [simple INI file](meteo-influx-gateway.ini.example):

```ini
[mqtt]
remote = "127.0.0.1"

[influxdb]
remote = "http://127.0.0.1:8086"
username = "meteo"
password = "meteo"
database = "meteo"
```

