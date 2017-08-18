# meteo
Weather and environmental monitoring solution

This project aims to provide a centralized enviromental and room monitoring system for different sensors.
On a central server runs the server instance, that collects all the sensor values from different nodes.
Client can attach to this server instance in order to read out the different readings of the sensors.

## Instructions

The project is currently in development and experimental. Build instructions are not yet given.

## Sensors

This folder contains the different measurement clients.

## Server

The server instance runs on a dedicated computer, that is the center for all the sensor readings.
All the sensors send their readings to this server instance and all clients collect data from this instance.

The sensor nodes can send the data via serial port, UDP or TCP sockets. A sensor node can hold mutliple sensors, that are arbitrary.

A client can connect via TCP and get individual readings or a push notification when a reading has changed
(configuration is planned, not yet implemented).
Currently push notifications about all sensors is implemented.

The server can write the readings into a MySQL database or into a single file.

## Nodes

Currently drivers for various Adafruit sensors have been implemented, as well as a skeleton code for building a program for the Raspberry Pi (in sensors)

A Arduino sketch for the JeeNode RoomNode is included as well (JeeNode)

## Client

An experimental Qt client is included as well in QMeteo. QtMeteo is the old, deprecated client that is not maintained anymore

QMeteo utilizes Qt5



