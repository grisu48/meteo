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
    go get github.com/mattn/go-sqlite3

## Configuration

Currently manually. See `meteod.toml`

## Storage

`meteod` stores all data in a Sqlite3 database. By default `meteod.db` is taken, but the filename can be configured in `meteod.toml`.

## Security

Pushing stations need a access `token`, that identifies where the data belongs to.

# Client

There is currently a very simple CLI client available: `meteo`

    meteo REMOTE
    
    $ meteo http://meteo-service.local/
      *   1 meteo-cluster          2019-05-14-17:24:01   22.51C|23.00 %rel| 95337hPa

## Requirements

Requires `go >= 1.9.x`

    go get "github.com/BurntSushi/toml"

## Configuration

Currently manually. See `meteod.toml`

# Nodes/Sensors

Currently I have some Adafruit sensors, a skeleton code for Raspberry Pi, a JeeNode RoomNode (that is not utilized anymore) and some EPS8266 projects.

This whole project is ongoing work, so nothing is production ready yet, as I'm still experimenting with lots of sensors.
