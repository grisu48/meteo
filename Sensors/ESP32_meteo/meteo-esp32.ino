#include <WiFi.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <PubSubClient.h>

#include <WebServer.h>

/* ==== CONFIGURE HERE ====================================================== */

#define SERIAL_BAUD 115200
#define LED_BUILTIN 2
#define N_SAMPLES 2         // Average report over this samples
#define SAMPLE_DELAY 5000   // Milliseconds between samples
#define SAMPLE_ALPHA 0.75   // Sampling average alpha


// TODO: Set your Wifi SSID and password

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// TODO: Configure your node here

#define NODE_ID 0
#define NODE_NAME "workroom"

// TODO: Set your MQTT server

#define MQTT_REMOTE "192.168.1.1"
#define MQTT_PORT 1883
#define MQTT_CLIENTID "meteo-" NODE_NAME
#define MQTT_TOPIC "home/" NODE_NAME

/* ========================================================================== */


BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// Current readings
static float temp(0), hum(0), pres(0);

static void led_toggle(const bool is_on) {
  if (is_on)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // Wait for serial port
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  led_toggle(false);

  // Init BME280
  Wire.begin();
  while(!bme.begin())
  {
    Serial.println("ERR: BME280");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("BME280");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("BMP280");
       break;
     default:
       Serial.println("ERR: Sensor");
  }
  read_bme280(temp,hum,pres);   // Initial read
  

  // Wifi connection
  while(true) {
    Serial.println("Wifi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("ERR: Wifi");
        delay(5000);
        continue;
    } else
      break;
  }
  Serial.print("Wifi OK "); Serial.println(WiFi.localIP());
  client.setServer(MQTT_REMOTE, MQTT_PORT);
  server.on("/", www_handle);
  server.on("/csv", csv_handle);
  server.on("/json", json_handle);
  server.begin();
}

int read_bme280(float &temp, float &hum, float &pres) {
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  return 0;
}


void reconnect_mqtt() {
  while (!client.connected()) {
    if (client.connect(MQTT_CLIENTID)) {
    } else {
      Serial.print("MQTT connect failed, rc=");
      Serial.print(client.state());
      delay(5000);    // wait 5 seconds to not flood the network
    }
  }
}

void report(float temp, float hum, float pres) {
  char message[256];
  snprintf(message, 255, "{\"node\":%d,\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f}", NODE_ID, temp, hum, pres);
  Serial.println(message);
  client.publish(MQTT_TOPIC, message);
}

void www_handle() {
  char html[512];
  snprintf(html, 512, "<!DOCTYPE html>\n<html>\n<body><h1>ESP32 Meteo Node</h1>\n<table><tr><td>Node</td><td>%d <b>%s</b></td></tr> <tr><td>Temperature</td><td>%.2f deg C</td><tr><td>Humidity</td><td>%.2f %% rel</td><tr><td>Pressure</td><td>%.2f hPa</td> </tr> </table>\n<p>Readings: <a href=\"/csv\">[csv]</a> <a href=\"/json\">[json]</a></p></body></html>", NODE_ID, NODE_NAME, temp, hum, pres);
  server.send(200, "text/html", html);
}

void csv_handle() {
  char csv[32];
  snprintf(csv, 32, "%.2f,%.2f,%.2f\n", temp, hum, pres);
  server.send(200, "text/csv", csv);
}

void json_handle() {
  char json[256];
  snprintf(json, 255, "{\"node\":%d,\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f}\n", NODE_ID, temp, hum, pres);
  server.send(200, "text/json", json);
}

void loop() {
  if (!client.connected()) { reconnect_mqtt(); }
  client.loop();
  server.handleClient();



  const long timestamp = millis();
  static long next_sample = timestamp + SAMPLE_DELAY;
  if(timestamp > next_sample) {
    next_sample = timestamp + SAMPLE_DELAY;
    float t(0), h(0), p(0);
    if(read_bme280(t, h, p) != 0) {
      Serial.println("ERR: BME280");
    } else {
      temp = SAMPLE_ALPHA*temp + (1.0-SAMPLE_ALPHA)*t;
      hum = SAMPLE_ALPHA*hum + (1.0-SAMPLE_ALPHA)*h;
      pres = SAMPLE_ALPHA*pres + (1.0-SAMPLE_ALPHA)*p;
  
      static int n = 0;
      if(++n >= N_SAMPLES) {
        report(temp,hum,pres);
        n = 0;
      }
    }
  }
  delay(100);
}
