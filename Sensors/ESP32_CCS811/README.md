# ESP32 - CCS811

Status: Operational

This is about getting the [Adafruit CCS811](https://www.adafruit.com/product/3566) digital gas sensor.

The sensor reads `eCO2` (equivalent calculated CO2) within a range of 400 to 8192 ppm (parts per million) and the `tVOC` (total volatile organic compound) concentration within a range of 0 to 1187 ppb (parts per billion). The temperature output does not relate to the actual temperature in the room but serves for internal calibration, so do not use it as temperature sensor.

* [CCS811_test.ino](CCS811_test.ino) - Simple CCS811 test sketchbook
* [CSS811-Node.ino](CSS811-Node.ino) - CCS811 MQTT Node
* [CSS811_BME280-Node.ino](CSS811_BME280-Node.ino) CCS811 + BME280 MQTT Node

## Weblinks

* [Adafruit CCS811 product page](https://www.adafruit.com/product/3566)
* [Github repo for Adafruit CCS811](https://github.com/adafruit/Adafruit_CCS811)