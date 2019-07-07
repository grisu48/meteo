/*
  AS3935 MOD-1016 Lightning Sensor Arduino test sketch
  Written originally by Embedded Adventures
  Modified by phoenix, 2019
*/

#define BAUD_RATE     115200
#define TUNE 6        // Tune value. Found on package of ch
#define NOISE 2       // Increase if too many false positives
#define BME_REAOUT  1000    // BME readout every x milliseconds

#include <Wire.h>
#include "AS3935.h"
#include <SPI.h>
#include <BME280I2C.h>

#define IRQ_PIN 2
#define CS_PIN 10

#define PIN_LIGHTNING 0
#define PIN_DISTURBER 0
#define PIN_NOISE 0
#define PIN_ERROR 0

volatile bool detected = false;

long lightnings = 0;
long noises = 0;
long disturbers = 0;

BME280I2C bme;

void recalibrate(bool tune = true) {
  if(tune) autoTuneCaps(IRQ_PIN);
  
  Serial.println("# TUNE\tIN/OUT\tNOISEFLOOR");
  Serial.print("# ");
  Serial.print(mod1016.getTuneCaps(), HEX);
  Serial.print("\t");
  Serial.print(mod1016.getAFE(), BIN);
  Serial.print("\t");
  Serial.println(mod1016.getNoiseFloor(), HEX);
  Serial.print("\n");  
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) {}
  Serial.println("INIT START");
  Serial.println("# Garden meteo sensor box");

  //I2C
  Wire.begin();

  // BME280
  while(!bme.begin())
  {
    Serial.println("ERR: BME280 not present");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("INFO: BME280");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("INFO: BMP280");
       break;
     default:
       Serial.println("ERR: Unknown BME sensor");
  }

  // Initialize MOD-1016
  Serial.println("INFO: Initialize MOD-1016");
  //recalibrate()
  mod1016.init(IRQ_PIN);
  mod1016.setTuneCaps(TUNE);
  //mod1016.setIndoors();
  mod1016.setOutdoors();
  mod1016.setNoiseFloor(NOISE); 
  Serial.println("# TUNE\tIN/OUT\tNOISEFLOOR");
  Serial.print("# ");
  Serial.print(mod1016.getTuneCaps(), HEX);
  Serial.print("\t");
  Serial.print(mod1016.getAFE(), BIN);
  Serial.print("\t");
  Serial.println(mod1016.getNoiseFloor(), HEX);
  Serial.print("\n");  

  pinMode(IRQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), alert, RISING);
  mod1016.getIRQ();
  Serial.println("INIT OK");
}
  
void loop() {
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }
  static long bme_timer = 0;
  if(millis() > bme_timer) {
    float temp(NAN), hum(NAN), pres(NAN);
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
    bme.read(pres, temp, hum, tempUnit, presUnit);
    Serial.print("BME "); Serial.print(temp);
    Serial.print(' '); Serial.print(hum);
    Serial.print(' '); Serial.print(pres);
    Serial.println();
    
    bme_timer = millis() + 1000;    // Readout
  }
}

void alert() {
  detected = true;
}

void translateIRQ(int irq) {
  Serial.print(millis());
  Serial.print(' ');
  int distance = 0;
  switch(irq) {
      case 1:
        Serial.println("NOISE");
        noises++;
        break;
      case 4:
        Serial.println("DISTURBER");
        disturbers++;
        break;
      case 8: 
        lightnings++;
        distance = mod1016.calculateDistance();

        Serial.print("LIGHTNING ");
        Serial.print(distance);
        Serial.println(" km");
        break;
      default:
        Serial.print("UNKNOWN INTERRUPT ");
        Serial.println(irq);
        return;
    }
}
