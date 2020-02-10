/***************************************************************************
  CSS811 meteo node (github.com/grisu48/meteo)

  This is a meteo node with a Adafruit CSS811 and BME280 sensor
  Published the readings via MQTT

  2020, Felix Niederwanger
  
 ***************************************************************************/
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_CCS811.h>

#define SERIAL_BAUD 115200
#define LED_BUILTIN 2
#define CSS811_WAKE_PIN 23
#define PUSH_DELAY 5000   // Milliseconds between data push

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

Adafruit_CCS811 ccs;

WiFiClient espClient;
PubSubClient client(espClient);

typedef struct ccs811_t {
  float t;
  float eco2;
  float tvoc;
} ccs811_t;
ccs811_t ccs811;


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
}

void loop() {

  if (!client.connected()) { reconnect_mqtt(); }
  client.loop();

  
  // Read whenever possible
  if(ccs.available()){
    float temp = ccs.calculateTemperature();
    if(!ccs.readData()){
      ccs811.eco2 = ccs.geteCO2();
      ccs811.tvoc = ccs.getTVOC();
      ccs811.t = temp;
    }
  }
  // Push when ready
  const long timestamp = millis();
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
  snprintf(message, 255, "{\"id\":%d,\"name\":\"%s\",\"eco2\":%.2f,\"tvoc\":%.2f}", NODE_ID, NODE_NAME, ccs811.eco2, ccs811.tvoc);
  Serial.println(message);
  client.publish(topic, message);
}
