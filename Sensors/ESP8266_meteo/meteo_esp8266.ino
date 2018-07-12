
/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266MQTTClient.h>
#include <ESP8266WiFi.h>
MQTTClient mqtt;

#include <Wire.h>
#include "Adafruit_HTU21DF.h"

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

/************************* WiFi Access Point *********************************/

// TODO: Put your WiFi Settings here
#define WLAN_SSID       ""
#define WLAN_PASS       ""


// Interval between publishes in milliseconds (Default value: 15 seconds)
#define DELAY 1000L * 15
// Node ID as string

// TODO: Define your node identifications (id and name)
#define NODE_ID "0"
// Name of this node
#define NODE_NAME "NodeMCU"

// TODO: Put your mosquitto server here
// Remote host in the format "IP:PORT" (e.g. "127.0.0.1:1883")
#define MQTT_HOST "0.0.0.0:1883"



void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("meteo Node " NODE_ID " starting up ... ");


  if (!htu.begin()) {
    Serial.println("CRITICAL: Couldn't find HTU21D-F sensor");
    while (1);
  }

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.print(WLAN_SSID); Serial.print("  ");

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected ("); Serial.print(WiFi.localIP()); Serial.println(")");

  // Connect mqtt
  mqtt.begin("mqtt://192.168.0.100:1883");
}

void loop() {
  mqtt.handle();

  static long timer = millis();
  const long m_time = millis();
  if ((m_time - timer) > DELAY) {
    float t = htu.readTemperature();
    float hum = htu.readHumidity();
    
    String str = "{\"node\":" NODE_ID ",\"name\":\"" NODE_NAME "\",\"t\":" + String(t) + ",\"hum\":" + String(hum) + 
      //",\"ccs_t\":" + String(ccs_t) +  ",\"eCO2\":" + String(ccs_eCo2) + ",\"tVOC\":" + String(ccs_tVOC) +
      "}";
    Serial.println(str);
    String topic = "meteo/" NODE_ID;
    mqtt.publish(topic, str);
    timer = m_time;
  }
  
}
