
#include "Adafruit_CCS811.h"
#include "Adafruit_Si7021.h"


Adafruit_CCS811 ccs;
Adafruit_Si7021 si7021 = Adafruit_Si7021();

void setup() {
  Serial.begin(9600);
  Serial.println("# Setting up sensors ... ");
  
  if(!ccs.begin()){
    Serial.println("ERROR: Cannot find CCS811 sensor");
    while(true);
  }

  if(!si7021.begin()) {
    Serial.println("ERROR: Cannot find Si7021 sensor");
    while(true);
  }

  
  // Calibrate temperature sensor of CCS811
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);
}

void loop() {
  float t1 = 0.0F, t2 = 0.0F;
  float etCO2 = -1.0F;
  float tVOC = -1.0F;
  float hum = -1.0F;
  if(ccs.available()) {
    t1 = ccs.calculateTemperature();
    if(!ccs.readData()){
        etCO2 = ccs.geteCO2();    // ppm
        tVOC = ccs.getTVOC();     // ppm
    }
  }

  t2 = si7021.readTemperature();
  hum = si7021.readHumidity();
  
  Serial.print("t1 = "); Serial.print(t1); Serial.print(" deg, ");
  Serial.print("etCO2 = "); Serial.print(etCO2); Serial.print(" ppm, ");
  Serial.print("tVOC = "); Serial.print(tVOC); Serial.print(" ppm, ");

  Serial.print("t2 = "); Serial.print(t2); Serial.print(" deg, ");
  Serial.print("hum = "); Serial.print(hum); Serial.print(" % rel");
  
  Serial.println();
  delay(1000);
}
