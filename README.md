# meteo

[![Build Status](https://travis-ci.org/grisu48/meteo.svg?branch=master)](https://travis-ci.org/grisu48/meteo)

Lightweight environmental monitoring solution

This project aims to provide a centralized environmental and room monitoring system for different sensors.
The meteo-daemon (`meteod`) runs on a centralised server instance, that collect all sensor data from different sensor nodes.

Client can attach to this server instance in order to read out the different readings of the sensors.

# Server

Requires `go >= 1.9.x` and the following repositories

    go get "github.com/BurntSushi/toml"
    go get "github.com/gorilla/mux"
    go get "github.com/mattn/go-sqlite3"
    go get "github.com/eclipse/paho.mqtt.golang"

A quick way of installing the requirements is

    $ make req                      # Installs requirements

## Configuration

Currently manually. See `meteod.toml` for information

## Storage

`meteod` stores all data in a Sqlite3 database. By default `meteod.db` is taken, but the filename can be configured in `meteod.toml`.

# Client

There is currently a very simple CLI client available: `meteo`

    meteo REMOTE
    
    $ meteo http://meteo-service.local/
      *   1 meteo-cluster          2019-05-14-17:24:01   22.51C|23.00 %rel| 95337hPa

## Build

Requires `go >= 1.9.x` and `"github.com/BurntSushi/toml"`

    $ make req-meteo
    $ make meteo

## Configuration

Currently manually. See `meteod.toml`

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

## Data-Push via HTTP

To push data via HTTP, you need a access `token`. The token identifies where the data belongs to.

The data is expected to be in the following `json` format

    { "token":"test", "T":32.0, "Hum": 33.1, "P":1013.5 }

Example-`curl` script to push data to station 5

    $ curl 'http://localhost:8802/station/5' -X POST -H "Content-Type: application/json" --data { "token":"test", "T":32.0, "Hum": 33.1, "P":1013.5 }