/* Lightning Node including BME280 sensor
 * 2018, Felix Niederwanger
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "AS3935.h"
#include <SPI.h>
#include <BME280I2C.h>

/* ==== Configuration section ==================================================== */

const char* ssid = "..SSID..";
const char* password = "..PASSWORD..";

#define MQTT_BME_NODEID "5"
#define IGNORE_LIGHNING_0 1   // I set to 1, we ignore lightnings at distance 0km, because they are likely false positives

#define MQTT_SERVER "192.168.0.100"
#define MQTT_PORT 1883
#define MQTT_TOPIC "meteo/lightning/" MQTT_BME_NODEID
#define MQTT_CLIENTID "meteo-lightning-" MQTT_BME_NODEID
#define MQTT_BME_NODENAME "Lightning"
#define MQTT_BME_TOPIC "meteo/" MQTT_BME_NODEID

#define PUB_NOISE 1     // Publish noise
#define PUB_DIST 1      // Publish distrurbers
#define NOISE_DIST_INTERVAL 2500       // Millisecond interval between reports of noise/distruber

#define BME_REPORT 10000 // Publish BME280 readout each x milliseconds

#define IRQ_PIN D3
#define CS_PIN 10

/* ==== Program section ========================================================== */

WiFiClient espClient;
PubSubClient mqtt(espClient);

BME280I2C bme;


volatile bool detected = false;

/* Counters */
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

  // setup MOD-1916
  Wire.begin();
  mod1016.init(IRQ_PIN);
  //Tune Caps, Set AFE, Set Noise Floor
  autoTuneCaps(IRQ_PIN);
  mod1016.setTuneCaps(4);   // In Lightning box tune caps = 4
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

  // Setup BME280
  int counter = 10;
  while(!bme.begin() && counter-- > 0) {
    Serial.println("BME280 init failure");
    delay(500);
  }
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Sensor = BME280");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Sensor = BMP280");
       break;
     default:
       Serial.println("ERR: Unknown sensor");
  }


  Serial.print("# Connecting WiFi ...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  
  Serial.println("OK");
  Serial.print("# Local IP: "); Serial.println(WiFi.localIP());
  Serial.println("Status: 1");
  Serial.println("# Ready - Waiting for triggers ...");
}

void loop() {
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }

  if(mqtt.connected()) {
    mqtt.loop();
  } else {
    MQTT_connect();
  }

  static long bme_ms = millis();
  long timestamp = millis();
  if(abs(timestamp-bme_ms) > BME_REPORT) {
    bme_report();
    bme_ms = timestamp;
  }
}

void bme_report() {
   float temp(NAN), hum(NAN), pres(NAN);
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(pres, temp, hum, tempUnit, presUnit);
  
  String buffer = "{\"node\":" + String(MQTT_BME_NODEID) + ",\"name\":" + String(MQTT_BME_NODENAME) + ",\"t\":";
  buffer += String(temp);
  buffer += ",\"hum\":";
  buffer += String(hum);
  buffer += ",\"p\":";
  buffer += String(pres);
  buffer += "}";
  mqtt.publish(MQTT_BME_TOPIC, buffer.c_str());

  Serial.print("temp  "); Serial.print(temp); Serial.print(" deg C");
  Serial.print("      hum "); Serial.print(hum); Serial.print(" % rel");
  Serial.print("      press "); Serial.print(pres); Serial.print(" hPa");
  Serial.println();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  while (!mqtt.connected()) {
    Serial.print("# Attempting MQTT connection ...");
    if (mqtt.connect(MQTT_CLIENTID)) {
      Serial.println(" ok ");
      Serial.println("MQTT: 0");
    } else {
      int rc = mqtt.state();
      Serial.print(" FAIL (");
      switch(rc) {
        case -4:
          Serial.print("Timeout"); break;
        case -3:
          Serial.print("Connection lost"); break;
        case -2:
          Serial.print("Connection failed"); break;
        case -1:
          Serial.print("Disconnected"); break;
        case 0:
          Serial.print("Connected"); break;
        case 1:
          Serial.print("Bad protocol"); break;
        case 2:
          Serial.print("Bad client id"); break;
        case 3:
          Serial.print("Unavailable"); break;
        case 4:
          Serial.print("Denied"); break;
        case 5:
          Serial.print("Unauthorized"); break;
      }
      Serial.print(mqtt.state());
      Serial.println(")");
      Serial.print("MQTT: "); Serial.println(rc);
      delay(2500);
    }
  }
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
        mqtt.publish(MQTT_TOPIC, buffer.c_str());
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
        mqtt.publish(MQTT_TOPIC, buffer.c_str());
#endif
      }
      last_disturber = timestamp;
      break;
    case 8:     // Lightning
      distance = mod1016.calculateDistance();
#if IGNORE_LIGHNING_0 == 1
      if(distance == 0.0F) {
        // Ignore head-on lightnigs because the interference with wifi makes them
        // indetectable and the false-alerts increases too much
      } else {
#endif
        lightnings++;
        
        Serial.print(timestamp);
        Serial.print(' ');
        Serial.print("LIGHTNING DETECTED ");
        Serial.print(distance);
        Serial.println(" km");
        buffer += ",\"lightning\":" + String(distance) + ",\"unit\":\"km\",";
        buffer += "\"lightnings\":" + String(lightnings) + "}";
        mqtt.publish(MQTT_TOPIC, buffer.c_str());
#if IGNORE_LIGHNING_0 == 1
      }
#endif
      break;
    default:
      Serial.print("UNKNOWN INTERRUPT ");
      Serial.println(irq);
      return;
  }
}
