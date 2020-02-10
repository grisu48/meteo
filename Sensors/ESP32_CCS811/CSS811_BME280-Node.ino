/***************************************************************************
  CSS811 meteo node (github.com/grisu48/meteo)

  This is a meteo node with a Adafruit CSS811 and BME280 sensor
  Published the readings via MQTT

  2020, Felix Niederwanger
  
 ***************************************************************************/
#include <WiFi.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <PubSubClient.h>
#include <Adafruit_CCS811.h>

#include <WebServer.h>

// TODO: Insert Wifi credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// TODO: Define Node ID and name
#define NODE_ID 1
#define NODE_NAME ""
// TODO MQTT (mosquitto) server
#define MQTT_REMOTE "192.168.0.100"
#define MQTT_PORT 1883
#define MQTT_CLIENTID "meteo-" NODE_NAME

#define SERIAL_BAUD 115200

#define LED_BUILTIN 2
#define CSS811_WAKE_PIN 23

#define PUSH_DELAY 5000   // Milliseconds between data push
#define N_SAMPLES 2         // Average report over this samples
#define SAMPLE_DELAY 5000   // Milliseconds between samples
#define SAMPLE_ALPHA 0.75   // Sampling average alpha


Adafruit_CCS811 ccs;
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

typedef struct ccs811_t {
  float t;
  float eco2;
  float tvoc;
} ccs811_t;
typedef struct bme280_t {
  float t;
  float hum;
  float p;
} bme280_t;
ccs811_t ccs811;
bme280_t bme280;


static void led_toggle(const bool is_on) {
  if (is_on)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // Wait for serial port
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
  read_bme280(bme280.t,bme280.hum,bme280.p);   // Initial read

  // Initialize CSS811
  pinMode(CSS811_WAKE_PIN, OUTPUT);
  digitalWrite(CSS811_WAKE_PIN, LOW);
  if(!ccs.begin()){
    Serial.println("ERR: CCS811 failed");
    while(1);
  }
  // calibrate temperature sensor
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);
  Serial.print("CSS811 calibrated: "); Serial.print(temp); Serial.println(" deg C");
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

void loop() {
  if (!client.connected()) { reconnect_mqtt(); }
  client.loop();
  server.handleClient();
  const long timestamp = millis();

  
  // Read from CCS811 whenever possible
  if(ccs.available()){
    float temp = ccs.calculateTemperature();
    if(!ccs.readData()){
      ccs811.eco2 = ccs.geteCO2();
      ccs811.tvoc = ccs.getTVOC();
      ccs811.t = temp;
    }
  }
  // Read from BME280 when necessary
  static long next_sample = timestamp + SAMPLE_DELAY;
  if(timestamp > next_sample) {
    next_sample = timestamp + SAMPLE_DELAY;
    float t(0), h(0), p(0);
    if(read_bme280(t, h, p) != 0) {
      Serial.println("ERR: BME280");
    } else {
      bme280.t = SAMPLE_ALPHA*bme280.t + (1.0-SAMPLE_ALPHA)*t;
      bme280.hum = SAMPLE_ALPHA*bme280.hum + (1.0-SAMPLE_ALPHA)*h;
      bme280.p = SAMPLE_ALPHA*bme280.p + (1.0-SAMPLE_ALPHA)*p;
    }
  }
  
  // Push when ready
  static long next_push = timestamp+PUSH_DELAY;
  if(timestamp > next_push) {
    next_push = timestamp+PUSH_DELAY;
    push_mqtt();
    led_toggle(true);
    delay(200);
    led_toggle(false);
  } else
    delay(100);
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
      Serial.println(client.state());
      delay(5000);    // wait 5 seconds to not flood the network
    }
  }
}

void push_mqtt() {
  char message[256];
  char topic[64];
  snprintf(topic, 64, "meteo/%d", NODE_ID);
  snprintf(message, 255, "{\"id\":%d,\"name\":\"%s\",\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f,\"eCO2\":%.0f,\"tVOC\":%.0f}", NODE_ID, NODE_NAME, bme280.t, bme280.hum, bme280.p, ccs811.eco2, ccs811.tvoc);
  Serial.println(message);
  client.publish(topic, message);
}



void www_handle() {
  char html[512];
  snprintf(html, 512, "<!DOCTYPE html>\n<html>\n<body><h1>ESP32 Meteo Node</h1>\n<table><tr><td>Node</td><td>%d <b>%s</b></td></tr> <tr><td>Temperature</td><td>%.2f deg C</td></tr> <tr><td>Humidity</td><td>%.2f %% rel</td></tr> <tr><td>Pressure</td><td>%.2f hPa</td></tr> <tr><td>eCO2</td><td>%.0f ppm</td></tr> <tr><td>tVOC</td><td>%.0f ppb</td></tr> </table>\n<p>Readings: <a href=\"/csv\">[csv]</a> <a href=\"/json\">[json]</a></p></body></html>", NODE_ID, NODE_NAME, bme280.t, bme280.hum, bme280.p, ccs811.eco2, ccs811.tvoc);
  server.send(200, "text/html", html);
}

void csv_handle() {
  char csv[32];
  snprintf(csv, 32, "%.2f,%.2f,%.2f,%.0f,%.0f\n", bme280.t, bme280.hum, bme280.p, ccs811.eco2, ccs811.tvoc);
  server.send(200, "text/csv", csv);
}

void json_handle() {
  char json[256];
  snprintf(json, 255, "{\"node\":%d,\"name\":\"%s\",\"t\":%.2f,\"hum\":%.2f,\"p\":%.2f,\"eCO2\":%.0f,\"tVOC\":%.0f}\n", NODE_ID, NODE_NAME, bme280.t, bme280.hum, bme280.p, ccs811.eco2, ccs811.tvoc);
  server.send(200, "text/json", json);
}
