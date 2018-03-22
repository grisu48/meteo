
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
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include "Adafruit_CCS811.h"

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_CCS811 ccs = Adafruit_CCS811();

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       ""
#define WLAN_PASS       ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "127.0.0.1"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""

#define NODE_ID         "0"             // Change this to the NODE ID
#define NODE_NAME       "Node"       // Change this to your NODE NAME


#define DELAY          2*1000L    // Publish every [milliseconds]

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// meteo publish path
Adafruit_MQTT_Publish meteo = Adafruit_MQTT_Publish(&mqtt, "meteo/" NODE_ID);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("meteo Node " NODE_ID " starting up ... ");


  if (!htu.begin()) {
    Serial.println("CRITICAL: Couldn't find HTU21D-F sensor");
    while (1);
  }
  if(!ccs.begin()){
    Serial.println("CRITICAL: Failed to start sensor! Please check your wiring.");
    while(1);
  }
  //calibrate temperature sensor
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  const long counter = millis() + DELAY;
  
  // Publish current readings
  float ccs_t = 0.0;
  float ccs_eCo2 = 0;
  float ccs_tVOC = 0;
  if(ccs.available()){
    ccs_t = ccs.calculateTemperature();
    if(!ccs.readData()) {
      ccs_eCo2 = ccs.geteCO2();
      ccs_tVOC = ccs.getTVOC();
    } else {
      Serial.println("ERROR: Error reading CCS811 sensor");
    }
  } else {
      Serial.println("ERROR: CCS811 appears to be not present");
  }


  float t = htu.readTemperature();
  float hum = htu.readHumidity();

  String str = "{\"node\":" NODE_ID ",\"name\":\"" NODE_NAME "\",\"t\":" + String(t) + ",\"hum\":" + String(hum) + 
    ",\"ccs_t\":" + String(ccs_t) + 
    ",\"eCO2\":" + String(ccs_eCo2) +
    ",\"tVOC\":" + String(ccs_tVOC) +
    "}";

  if (!meteo.publish(str.c_str())) {
    Serial.println(F("Publish failed"));
  } else {
    Serial.println(str);
  }

  long delay_ms = counter - millis();
  if(delay_ms > 0)
    delay(delay_ms);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if(! mqtt.ping()) {
    Serial.println("MQTT Ping timeout");
    mqtt.disconnect();
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.println("Connecting MQTT... ");

  long tries = 0;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      Serial.print("  Error: "); Serial.println(mqtt.connectErrorString(ret));
      mqtt.disconnect();
      tries++;
      long delay_ms = 1000L*tries*tries;
      if(delay_ms>30000L) delay_ms = 30000L;
      delay(delay_ms);
  }
  Serial.print("MQTT Connected");
  if(tries>0) { Serial.print(tries); Serial.print(" retries)"); }
  Serial.println();
}
