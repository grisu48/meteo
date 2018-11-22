# meteo
Weather and environmental monitoring solution

This project aims to provide a centralized enviromental and room monitoring system for different sensors.
On a central server runs the server instance, that collects all the sensor values from different nodes.
Client can attach to this server instance in order to read out the different readings of the sensors.

## Instructions

The project is currently in development and experimental. Build instructions are not yet given.

## Server

The server instance runs on a dedicated computer, that stores all the sensor readings.
All the sensors send their readings to this server instance via MQTT.

Currently the server is written in go and found in the `server` directory.

## Nodes/Sensors

Currently I have some Adafruit sensors, a skeleton code for Raspberry Pi, a JeeNode RoomNode (that is not utilized anymore) and some EPS8266 projects.

This whole project is ongoing work, so nothing is production ready yet, as I'm still experimenting with lots of sensors.

# Clients

## QMeteo

An experimental Qt client is included as well in QMeteo (qt5).

## Android

An android client is under development. See the `Android` branch.
