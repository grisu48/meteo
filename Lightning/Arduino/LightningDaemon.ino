/*

Copyright (c) 2017, Embedded Adventures
All rights reserved.

Contact us at source [at] embeddedadventures.com
www.embeddedadventures.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

- Neither the name of Embedded Adventures nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.

AS3935 MOD-1016 Lightning Sensor Arduino test sketch
  Written originally by Embedded Adventures
  Modified by phoenix, 2017

*/




#include <Wire.h>
#include "AS3935.h"
#include <SPI.h>

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

// Timers for LEDS
long led_t_lightning = 0L;
long led_t_disturber = 0L;
long led_t_noise = 0L;
// LEDs on or off
bool led_lightning = false;
bool led_disturber = false;
bool led_noise = false;
bool led_error = false;

static void led_handle(const long t, bool *led_status, long *timer, int pin) {
  if (*led_status == false) return;
  else {
    if (t > *timer) {
      *timer = 0L;
      *led_status = false;
      if(pin != 0) {
        digitalWrite(pin, LOW);   // Turn off
      }
      Serial.print("# LED OFF (");
      Serial.print(pin);
      Serial.println(')');
    }
  }
}

static void led_on(int pin, bool *led_status, long *timer, long duration) {
  if(pin > 0)
    digitalWrite(pin, HIGH);
  *led_status = true;
  *timer = (millis() + duration);
  
    Serial.print("# LED ON (");
    Serial.print(pin);
    Serial.println(')');
}

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
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("# MOD-1016 (AS3935) Lightning Sensor Monitor System");

  //I2C
  Wire.begin();
  mod1016.init(IRQ_PIN);

  //Tune Caps, Set AFE, Set Noise Floor
  autoTuneCaps(IRQ_PIN);
  
  //mod1016.setTuneCaps(9);
  mod1016.setIndoors();
  //mod1016.setOutdoors();
  mod1016.setNoiseFloor(5);
 
  
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
  Serial.println("# Interrupt setup completed");
}

void loop() {
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }


  // LEDs
  const long t = millis();
  // XXX: TODO: Timer long overflow
  led_handle(t, &led_lightning, &led_t_lightning, PIN_LIGHTNING);
  led_handle(t, &led_disturber, &led_t_disturber, PIN_DISTURBER);
  led_handle(t, &led_noise, &led_t_noise, PIN_NOISE);
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
        Serial.println("NOISE DETECTED");
        noises++;
        led_on(PIN_NOISE, &led_noise, &led_t_noise, 1000);
        break;
      case 4:
        Serial.println("DISTURBER DETECTED");
        disturbers++;
        led_on(PIN_DISTURBER, &led_disturber, &led_t_disturber, 200);
        break;
      case 8: 
        led_on(PIN_LIGHTNING, &led_lightning, &led_t_lightning, 1000);
        lightnings++;
        distance = mod1016.calculateDistance();


        
        Serial.print("LIGHTNING DETECTED ");
        if(distance < 0) {
          Serial.println("??? km");
        } else if(distance == 0) {
          // Overhead. Special case for LED in future
          Serial.println("0 km");
        } else {
          Serial.print(distance);
          Serial.println(" km");
        }
        break;
      default:
        Serial.print("UNKNOWN INTERRUPT ");
        Serial.println(irq);
        return;
    }
    //Serial.print(noises);
    //Serial.print(' ');
    //Serial.print(disturbers);
    //Serial.print(' ');
    //Serial.print(lightnings);
    //Serial.println();
}

#if 0
void printDistance() {
  int distance = mod1016.calculateDistance();
  if (distance == -1) {
    Serial.println("# Lightning out of range");
    Serial.print(": -1");
    Serial.println();
  } else if (distance == 1) { 
    Serial.println("# Distance not in table");
  } else if (distance == 0) {
    Serial.println("# Lightning overhead");
    Serial.print(": 0");
    Serial.println();
  } else {
    Serial.print("# Lightning ~");
    Serial.print(distance);
    Serial.println("km away");  
    Serial.print("LIGTHNING ");
    Serial.print(distance);
    Serial.println();
  }
}
#endif

