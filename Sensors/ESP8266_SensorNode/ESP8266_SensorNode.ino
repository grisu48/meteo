#include <ESP8266WiFi.h>

#include <Wire.h>

#include "Adafruit_HTU21DF.h"
#include "Adafruit_MCP9808.h"
#include "Adafruit_LSM303.h"
#include "Adafruit_BMP085.h"
#include "Adafruit_TSL2561.h"


const char* ssid = "feldspaten";
const char* password = "4theGloryOfThe3ofUs";

// Quite simple server
WiFiServer server(80);

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
bool htu_ready;
Adafruit_MCP9808 mcp9808 = Adafruit_MCP9808();
bool mcp9808_ready;
Adafruit_BMP085 bmp180 = Adafruit_BMP085();
bool bmp180_ready;
Adafruit_LSM303 lsm303 = Adafruit_LSM303();
bool lsm303_ready;
TSL2561 tsl2561 = TSL2561(TSL2561_ADDR_LOW);
bool tsl2561_ready;

void setup() 
{
  Serial.begin(57600);
  delay(100);
  
  // We start by connecting to a WiFi network
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start sensors
  htu_ready = htu.begin();
  if(!htu_ready)
    Serial.println("ERROR: HTU21D-f not ready");
  mcp9808_ready = mcp9808.begin();
  if(!mcp9808_ready)
    Serial.println("ERROR: MCP9808 not ready");
  bmp180_ready = bmp180.begin();
  if(!bmp180_ready)
    Serial.println("ERROR: BMP180 not ready");
  lsm303_ready = lsm303.begin();
  if(!lsm303_ready)
    Serial.println("ERROR: LSM303 not ready");
  tsl2561_ready = tsl2561.begin();
  if(!tsl2561_ready)
    Serial.println("SERROR: TSL2561 not ready");
  
  // Start webserver
  server.begin();
  Serial.println("Webserver up and running");
  
}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    delay(100);
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Start header, in anticipation that the request is valid

  // Match request
  Serial.println(req);
  if(req.indexOf("GET / ") != -1 || req.indexOf("GET /index.html") != -1 || req.indexOf("GET index.html") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n\r\n";
    response += "<!DOCTYPE HTML>\r\n<html>\r\n";
    
    response += "<head><title>ESP8266</title></head><body><h1>ESP 8266</h1><p>This is the main page of the ESP8266 Node. Welcome!</p>\r\n";
    response += "<h2>Sensors</h2>\r\n";
    response += "<p>Go to the <a href=\"sensors\">sensors page</a> for a full readout or check the <a href=\"status\">status</a> of the sensors</p>";
    response += "<ul>";
    if(htu_ready)
      response += "<li><a href=\"htu21df\">HTU21D-f (Humidity/temperature sensor)</a></li>";
    if(mcp9808_ready)
      response += "<li><a href=\"mcp9808\">MCP9808 (high accuracy temperature sensor)</a></li>";
    if(lsm303_ready)
      response += "<li><a href=\"lsm303\">LSM303DLHC (Magnetic/Accelerometer sensor)</a></li>";
    if(bmp180_ready)
      response += "<li><a href=\"bmp180\">BMP180 (Pressure/temperature sensor)</a></li>";
    if(tsl2561_ready)
      response += "<li><a href=\"tsl2561\">TSL2561 (Luminosiry sensor)</a></li>";
    response += "</ul>";
    response += "<p>2017, Felix Niederwanger</p>";
    response += "</body></html>";
    client.print(response);
  } else if(req.indexOf("GET /readings ") != -1 || req.indexOf("GET /sensors ") != -1) {
    // Get all readings

    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";


    if(htu_ready) {
      response += "HT21DF: ";
      response += String(htu.readTemperature())  + " deg C  ";
      response += String(htu.readHumidity())  + " % rel\r\n";
    } 

    if(mcp9808_ready) {
      response += "MCP9808: ";
      response += String(mcp9808.readTempC())  + " deg C\r\n";
    } 

    if(lsm303_ready) {
      response += "LSM303DLHC: ";
      lsm303.read();
      lsm303AccelData acc = lsm303.accelData;
      lsm303MagData mag = lsm303.magData;
      float acc_total = sqrt(acc.x*acc.x + acc.y * acc.y + acc.z * acc.z);
      float mag_total = sqrt(mag.x*mag.x + mag.y * mag.y + mag.z * mag.z);
      response += "Magnetic field : " + String(mag_total) + " (" + String(mag.x) + " " + String(mag.y) + " " + String(mag.z) + ")";
      response += "Acceleration   : " + String(acc_total) + " (" + String(acc.x) + " " + String(acc.y) + " " + String(acc.z) + ")\r\n";
    } 
    
    if(bmp180_ready) {
      response += "BMP180: ";
      response += String(bmp180.readTemperature())  + " deg C  ";
      response += String(bmp180.readPressure())  + " hPa C\r\n";
    } 

    if(tsl2561_ready) {
      response += "TSL2561: ";
      float total = tsl2561.getFullLuminosity();
      float infrared = tsl2561.getLuminosity(1);
      float visible = total - infrared;
      response += String(visible)  + " visible, ";
      response += String(infrared)  + " ir (";
      response += String(total)  + " total)\r\n";
    } 

    
    client.print(response);
  } else if(req.indexOf("GET /htu21d-f ") != -1 || req.indexOf("GET /htu21df ") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";
    if(htu_ready) {
      response += String(htu.readTemperature())  + " deg C\r\n";
      response += String(htu.readHumidity())  + " % rel";
    } else
      response += "Sensor not ready";
    client.print(response);
  } else if(req.indexOf("GET /mcp9808 ") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";
    if(mcp9808_ready) {
      response += String(mcp9808.readTempC())  + " deg C\r\n";
    } else
      response += "Sensor not ready";
    client.print(response);
  } else if(req.indexOf("GET /lsm303 ") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";
    if(lsm303_ready) {
      lsm303.read();
      lsm303AccelData acc = lsm303.accelData;
      lsm303MagData mag = lsm303.magData;
      float acc_total = sqrt(acc.x*acc.x + acc.y * acc.y + acc.z * acc.z);
      float mag_total = sqrt(mag.x*mag.x + mag.y * mag.y + mag.z * mag.z);
      response += "Magnetic field : " + String(mag_total) + " (" + String(mag.x) + " " + String(mag.y) + " " + String(mag.z) + ")\r\n";
      response += "Acceleration   : " + String(acc_total) + " (" + String(acc.x) + " " + String(acc.y) + " " + String(acc.z) + ")\r\n";
    } else
      response += "Sensor not ready";
    client.print(response);
  } else if(req.indexOf("GET /bmp180 ") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";
    if(bmp180_ready) {
      response += String(bmp180.readTemperature())  + " deg C\r\n";
      response += String(bmp180.readPressure())  + " hPa C\r\n";
    } else
      response += "Sensor not ready";
    client.print(response);
  } else if(req.indexOf("GET /tsl2561 ") != -1) {
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n\r\n";
    if(tsl2561_ready) {
      float total = tsl2561.getFullLuminosity();
      float infrared = tsl2561.getLuminosity(1);
      float visible = total - infrared;
      response += String(visible)  + " visible, ";
      response += String(infrared)  + " ir (";
      response += String(total)  + " total)\r\n";
    } else
      response += "Sensor not ready";
    client.print(response);
  } else {
    // Write 404
    String response = "HTTP/1.1 404\r\n";
    response += "Content-Type: text/html\r\n\r\n";
    response += "<!DOCTYPE HTML>\r\n<html>\r\n";
    response += "<html><head><tilte>404 - Not found</title></head><body><h1>404 - Not found</h1><p>The requested uri has not been found</p>";
    response += "<p>Perhaps <a href=\"index.html\">got back to the default page</a>?</p>";
    response += "</body></html>";
    client.print(response);
  }
}
