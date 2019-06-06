# meteo

[![Build Status](https://travis-ci.org/grisu48/meteo.svg?branch=master)](https://travis-ci.org/grisu48/meteo)

Weather and environmental monitoring solution

This project aims to provide a centralized enviromental and room monitoring system for different sensors.
On a central server runs the server instance, that collects all the sensor values from different nodes.
Client can attach to this server instance in order to read out the different readings of the sensors.

# Server

Requires `go >= 1.9.x`

## Configuration

Currently manually. See `meteod.toml`

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
    go get "github.com/go-sql-driver/mysql"
    go get "github.com/fatih/color"


## Configuration

Currently manually. See `meteod.toml`

# Nodes/Sensors

Currently I have some Adafruit sensors, a skeleton code for Raspberry Pi, a JeeNode RoomNode (that is not utilized anymore) and some EPS8266 projects.

This whole project is ongoing work, so nothing is production ready yet, as I'm still experimenting with lots of sensors.
