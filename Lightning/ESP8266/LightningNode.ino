

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "AS3935.h"
#include <SPI.h>



const char* ssid = "";
const char* password = "";

#define MAX_CLIENTS 8


// Server instance
WiFiServer server(1885);			// Or any other port
WiFiClient* clients[MAX_CLIENTS] = { NULL };



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
  server.begin();
  Serial.println("connected ");
  Serial.print("# IP : ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Status: 1");
  Serial.println("# Ready - Waiting for triggers ...");
}

void loop() {
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }

  // Accept clients
  WiFiClient newClient = server.available();
  if(newClient) {
    for(int i=0; i<MAX_CLIENTS; i++) {
      if(clients[i] == NULL || !clients[i]->connected()) {
        if(clients[i] != NULL) delete clients[i];
        clients[i] = new WiFiClient(newClient);
        Serial.println("# Client attached");
        break;
      }
    }
  }
}

static void sendClients(const String &buffer) {
  for(int i=0; i<MAX_CLIENTS; i++) {
    if(clients[i] == NULL)
      continue;
    else {
      if(clients[i]->connected()) {
        clients[i]->print(buffer);
        clients[i]->print('\n');
        clients[i]->flush();
      } else {
        delete clients[i];
        clients[i] = NULL;
      }
    }
  }
}

void alert() {
  detected = true;
  //Serial.println("# ALERT");
}

void translateIRQ(int irq) {
  Serial.print(millis());
  Serial.print(' ');
  int distance = 0;
  String buffer = String(millis()) + " ";
  switch (irq) {
    case 1:
      Serial.println("NOISE DETECTED");
      buffer += "NOISE DETECTED";
      sendClients(buffer);
      noises++;
      break;
    case 4:
      Serial.println("DISTURBER DETECTED");
      buffer += "DISTURBER DETECTED";
      sendClients(buffer);
      disturbers++;
      break;
    case 8:
      lightnings++;
      distance = mod1016.calculateDistance();

      Serial.print("LIGHTNING DETECTED ");
      Serial.print(distance);
      Serial.println(" km");
      buffer += "LIGHTNING " + String(distance);
      sendClients(buffer);
      break;
    default:
      Serial.print("UNKNOWN INTERRUPT ");
      Serial.println(irq);
      return;
  }
}

