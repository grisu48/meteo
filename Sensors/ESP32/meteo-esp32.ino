#include <BME280I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include <WebServer.h>

/* ==== CONFIGURE HERE ====================================================== */

#define SERIAL_BAUD 115200
#define LED_BUILTIN 2
#define N_SAMPLES 2       // Average report over this samples
#define SAMPLE_DELAY 5000 // Milliseconds between samples
#define SAMPLE_ALPHA 0.75 // Sampling average alpha

// TODO: Set your Wifi SSID and password

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_RECONNECT_DELAY 10000 // Reconnection delay in milliseconds

// TODO: Configure your node here

#define NODE_ID 1
#define NODE_NAME "outdoors"

// TODO: Set your MQTT server

#define MQTT_REMOTE "192.168.0.1"
#define MQTT_PORT 1883
#define MQTT_CLIENTID "meteo-" NODE_NAME

/* ========================================================================== */

BME280I2C
    bme; // Default : forced mode, standby time = 1000 ms
         // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// Current readings
static float temp(0), hum(0), pres(0);

static void led_toggle(const bool is_on) {
  digitalWrite(LED_BUILTIN, is_on ? HIGH : LOW);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
  } // Wait for serial port

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  led_toggle(false);

  // Init BME280
  Wire.begin();
  while (!bme.begin()) {
    Serial.println("ERR: BME280");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch (bme.chipModel()) {
  case BME280::ChipModel_BME280:
    Serial.println("BME280");
    break;
  case BME280::ChipModel_BMP280:
    Serial.println("BMP280");
    break;
  default:
    Serial.println("ERR: Sensor");
  }
  read_bme280(temp, hum, pres); // Initial read

  // Wifi connection
  WiFi.onEvent(wifi_connected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(wifi_got_ip, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(wifi_disconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);
  wifi_connect();
  client.setServer(MQTT_REMOTE, MQTT_PORT);
  server.on("/", www_handle);
  server.on("/csv", csv_handle);
  server.on("/json", json_handle);
  server.begin();
}

void wifi_connect() {
  while (true) {
    Serial.println("Wifi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("ERR: Wifi");
      delay(5000);
      continue;
    } else {
      break;
    }
  }
}

void wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Wifi: Connected");
}

void wifi_got_ip(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("WiFi lost: ");
  Serial.println(info.disconnected.reason);
  wifi_connect();
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
      Serial.print("MQTT connect failed: error code ");
      Serial.print(client.state());
      delay(5000); // wait 5 seconds to not flood the network
    }
  }
}

void report(float temp, float hum, float pres) {
  char message[256];
  char topic[64];
  snprintf(topic, 64, "meteo/%d", NODE_ID);
  snprintf(message, 255,
           "{\"id\":%d,\"name\":\"%s\",\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f}",
           NODE_ID, NODE_NAME, temp, hum, pres);
  Serial.println(message);
  client.publish(topic, message);
}

void www_handle() {
  char html[512];
  snprintf(
      html, 512,
      "<!DOCTYPE html>\n<html>\n<body><h1>ESP32 Meteo "
      "Node</h1>\n<table><tr><td>Node</td><td>%d <b>%s</b></td></tr> "
      "<tr><td>Temperature</td><td>%.2f deg C</td></tr> "
      "<tr><td>Humidity</td><td>%.2f %% rel</td></tr> "
      "<tr><td>Pressure</td><td>%.2f hPa</td></tr> "
      "<tr><td>Pressure</td><td>%.2f hPa</td></tr> </table>\n<p>Readings: <a "
      "href=\"/csv\">[csv]</a> <a href=\"/json\">[json]</a></p></body></html>",
      NODE_ID, NODE_NAME, temp, hum, pres);
  server.send(200, "text/html", html);
}

void csv_handle() {
  char csv[32];
  snprintf(csv, 32, "%.2f,%.2f,%.2f\n", temp, hum, pres);
  server.send(200, "text/csv", csv);
}

void json_handle() {
  char json[256];
  snprintf(json, 255,
           "{\"node\":%d,\"name\":\"%s\",\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f}\n",
           NODE_ID, NODE_NAME, temp, hum, pres);
  server.send(200, "text/json", json);
}

void loop() {
  // mqtt reconnect
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  server.handleClient();

  // Sensor readout
  const long timestamp = millis();
  static long next_sample = timestamp + SAMPLE_DELAY;
  if (timestamp > next_sample) {
    next_sample = timestamp + SAMPLE_DELAY;
    float t(0), h(0), p(0);
    if (read_bme280(t, h, p) != 0) {
      Serial.println("ERR: BME280");
    } else {
      temp = SAMPLE_ALPHA * temp + (1.0 - SAMPLE_ALPHA) * t;
      hum = SAMPLE_ALPHA * hum + (1.0 - SAMPLE_ALPHA) * h;
      pres = SAMPLE_ALPHA * pres + (1.0 - SAMPLE_ALPHA) * p;

      static int n = 0;
      if (++n >= N_SAMPLES) {
        report(temp, hum, pres);
        n = 0;
      }
    }
  }
  delay(100);
}
