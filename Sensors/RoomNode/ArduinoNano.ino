/***************************************************************************
  This project uses the Adafruit BME280 sensor for
  humidity, temperature & pressure sensor

  The expected output format is 
  "ROOM 4 100 50 20 0" for Roomnode 4 (id = 4),
  with light = 100, humidity = 50 % rel, temperature = 20 degree C, and
  good battery (battery = 0)

  Modify for your needs :-)
 ***************************************************************************/

// ID of this node
#define ROOM_ID 8
// Serial BAUD rate (default: 57600)
#define BAUD_RATE 57600
// Delay between readings
#define DELAY 2000
// Smoothing value
#define ALPHA 0.9


#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

Adafruit_BME280 bme; // I2C

static float temperature = 0.0F;
static float humidity = 0.0F;
static float pressure = 0.0F;
static float light = 0.0F;    // Light, if sensor is available

/** Prints the current values */
void printRoomNode(void) {
  
  // e.g. "ROOM 4 100 50 20 0" for Roomnode 4 (id = 4),
  Serial.print("ROOM ");
  Serial.print(ROOM_ID);
  Serial.print(" ");
  Serial.print(light);
  Serial.print(" ");
  Serial.print(humidity);
  Serial.print(" ");
  Serial.print(temperature*10.0F);
  Serial.print(" ");
  Serial.print("0");
  Serial.println();
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.print("# ROOM Node ");
  Serial.print(ROOM_ID);
  Serial.println();

  if (!bme.begin()) {
    Serial.println("# ERROR: Could not find a valid BME280 sensor");
    while (1);
  } else {
    Serial.println("# Setting up ... ");
    temperature = bme.readTemperature();
    pressure = bme.readPressure() / 100.0F;
    humidity = bme.readHumidity();
    printRoomNode();
  }
}


void loop() {
    float l_temperature = bme.readTemperature();
    float l_pressure = bme.readPressure() / 100.0F;
    float l_humidity = bme.readHumidity();

    temperature = ALPHA * temperature + (1.0 - ALPHA) * l_temperature;
    pressure = ALPHA * pressure + (1.0 - ALPHA) * l_pressure;
    humidity = ALPHA * humidity + (1.0 - ALPHA) * l_humidity;

    printRoomNode();
    delay(DELAY);
}
