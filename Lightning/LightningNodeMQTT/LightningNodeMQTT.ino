

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "AS3935.h"
#include <SPI.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/* ==== Configuration section ==================================================== */

// TODO: Put in your configuration first!
const char* ssid = "";
const char* password = "";

#define MQTT_SERVER ""
#define MQTT_PORT 1883
#define MQTT_TOPIC "/meteo/lightning/1"

#define PUB_NOISE 1     // Publish noise
#define PUB_DIST 1      // Publish distrurbers
#define NOISE_DIST_INTERVAL 2500       // Millisecond interval between reports of noise/distruber


// Mosquitto instance
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, "", "");
Adafruit_MQTT_Publish mqtt_publish = Adafruit_MQTT_Publish(&mqtt, "" MQTT_TOPIC);


#define IRQ_PIN D3
#define CS_PIN 10

volatile bool detected = false;

long lightnings = 0;
long noises = 0;
long disturbers = 0;

void recalibrate(bool tune = true) {
  if (tune) autoTuneCaps(IRQ_PIN);

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
  delay(500);
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


  Serial.print("# Connecting WiFi ...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("Status: 1");
  Serial.println("# Ready - Waiting for triggers ...");
}

void loop() {
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }

  MQTT_connect();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}


void alert() {
  detected = true;
  //Serial.println("# ALERT");
}

void translateIRQ(int irq) {
  const long timestamp = millis();
  int distance = 0;
  String buffer = "{\"millis\":" + String(timestamp);

  // Only publish disturbers 1 per second
  static long last_disturber = millis();
  long t_diff_dist = timestamp - last_disturber;
  
  switch (irq) {
    case 1:     // Noise
      noises++;
      if(t_diff_dist > NOISE_DIST_INTERVAL) {
        Serial.print(timestamp);
        Serial.print(' ');
        Serial.println("NOISE DETECTED");
#if PUB_NOISE == 1
        buffer += ",\"noise\":" + String(noises) + "}";
        mqtt_publish.publish(buffer.c_str());
#endif
      }
      last_disturber = timestamp;
      break;
    case 4:     // Disturber
      disturbers++;
      if(t_diff_dist > NOISE_DIST_INTERVAL) {
        Serial.print(timestamp);
        Serial.print(' ');
        Serial.println("DISTURBER DETECTED");
#if PUB_DIST == 1
        buffer += ",\"disturbers\":" + String(disturbers) + "}";
        mqtt_publish.publish(buffer.c_str());
#endif
      }
      last_disturber = timestamp;
      break;
    case 8:     // Lightning
      distance = mod1016.calculateDistance();
      if(distance == 0.0F) {
        // Ignore head-on lightnigs because the interference with wifi makes them
        // indetectable and the false-alerts increases too much
      } else {
        lightnings++;
        
        Serial.print(timestamp);
        Serial.print(' ');
        Serial.print("LIGHTNING DETECTED ");
        Serial.print(distance);
        Serial.println(" km");
        buffer += ",\"lightning\": " + String(distance) + ",\"unit\":\"km\",";
        buffer += "\"lightnings\":" + String(lightnings) + "}";
        mqtt_publish.publish(buffer.c_str());
      }
      break;
    default:
      Serial.print("UNKNOWN INTERRUPT ");
      Serial.println(irq);
      return;
  }
}

