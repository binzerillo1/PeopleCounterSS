#include <Wisol.h>

/*
   Project: Arduino People counter using a VL53L0X sensor 
   Author:  FMT Brian Inzerillo

   This project will count toward a running total all the entrances and exits through a 
   doorway. It will do this by continuously running a distance ranging sensor, and 
   when the sensor is tripped it will start collecting data and do so until the door 
   is unobstructed. At this point, it will take the dataset and find the slope using a
   Least Squares Regression and Cramer's Rule to simplify the calculation. Based on 
   whether the slope is negative or postive, the person walked inside or outside. This
   is due to the angle that the sensor is at. 
*/

#include <LiquidCrystal.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <EEPROM.h>

WisolSigfox sf(8, 9);
VL53L0X sensor;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int peopleCount;
int peopleIn;
int peopleOut;
int lastCount;

volatile unsigned long current_ms = 0;
volatile unsigned long LineRefresh_ms = 500;
volatile unsigned long lastRefresh = 0;
volatile unsigned long last_person_ms = 0;
const long transmitTime = 60*1000;
 
const int reset_pin = 6;
#define sensor_address 60

int times[100];
  
void setup() {
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  
  for(int k = 1; k < 101; k++) {
    times[k-1] = k;
  }
  
  Wire.begin();

  int peopleCount = 0;
  peopleIn = 0;
  peopleOut = 0;
  Serial.begin(9600);
  Serial.println("setting up.");
  lcd.begin(16, 2);
  lcd.clear();

  pinMode(reset_pin, OUTPUT);
  digitalWrite(reset_pin, LOW);
  digitalWrite(reset_pin, HIGH);
  delay(250);
  sensor.setAddress((uint8_t)sensor_address);
Serial.println("HERE.");
  sensor.init(true);
  Serial.println("Starting up");

  sensor.setTimeout(100);
  Serial.println("ready.");
  sf.begin();
  char stringout[8];
  sprintf(stringout, "%04d", peopleCount);  
  sf.transmit(stringout, strlen(stringout));
  lastCount = peopleCount;
  sf.deepSleep();
}

void loop() {

  current_ms = millis();
  
  int distance = sensor.readRangeSingleMillimeters();
  //  Serial.println(distance);
  if(distance < 8000) {
    int i = 0;
    long set[100] = {};
    //Serial.println();
    while(distance < 8000) {
     last_person_ms = millis();
     set[i] = distance;
     //Serial.println();
     i++;
     delay(20);
     distance = sensor.readRangeSingleMillimeters();
    }
  //  Serial.print(i);
   // Serial.println(" values taken.");
    long sumTimeDist = 0;
    long sumTime = 0;
    long sumDist = 0;
    long sumTimeTime = 0;
    long slope = 0;
    for(int j = 0; j < i; j++) {
      sumTimeDist += set[j]*times[j];
      sumTime += times[j];
      sumDist += set[j];
      sumTimeTime += times[j]*times[j];
      //Serial.print("Time: ");
      //Serial.print(times[j]);
      Serial.print("  Dist: ");
      Serial.println(set[j]);
    }
    slope = (i*sumTimeDist - (sumTime*sumDist))/(i*sumTimeTime - (sumTime*sumTime));
//    Serial.print("sumTimeDist of ");    Serial.println(sumTimeDist);   
//    Serial.print("sumTime of ");    Serial.println(sumTime);    
//    Serial.print("sumDist of ");    Serial.println(sumDist);   
//    Serial.print("sumTimeTime of ");    Serial.println(sumTimeTime);
    
    Serial.print("Slope of ");
    Serial.println(slope);
    if(slope > 2) {
      peopleCount++;
      peopleIn++;
      Serial.print("Entered. Count = ");
      Serial.println(peopleCount);
    }
    if(slope < -2) {
      peopleCount--;
      peopleOut++;
      Serial.print("Left. Count = ");
      Serial.println(peopleCount);
    }
  updateLCDLine2();
  } 
  else {
    delay(10);
  }
  
  if((millis() - last_person_ms > transmitTime) && peopleCount != lastCount) {
     sf.deepSleepWakeup();
     char stringout[8];
     sprintf(stringout, "%04d", peopleCount);  
     sf.transmit(stringout, strlen(stringout));
     lastCount = peopleCount;
     sf.deepSleep();
  }
  
  if(current_ms - lastRefresh > LineRefresh_ms) {
    updateLCDLine1();
    updateLCDLine2();
  }
}

void updateLCDLine1() {
  // Update the title bar of the LCD
  lcd.setCursor(0, 0);
  lcd.print("FMT PPL Counter");
}

void updateLCDLine2() {
  lcd.setCursor(0, 1);
  lcd.print("Count:" + RightJustify(peopleCount, 3));
  lastRefresh = millis();
}

String RightJustify(int number, int NumPlaces) {
  String num_string = String(number);
  String space_pad = "";
  int pad = NumPlaces - num_string.length();
  if (pad >= 0) {
    for (int i = 0; i < pad; i++) {
      space_pad = space_pad + " ";
    }
    return (space_pad + num_string);
  }
  else //(pad < 0)
  { //keep just the (NumPlaces-1) least significant digits, make the first an E to indicate more than displayable count
    return ("E" + num_string.substring(num_string.length() - 3, num_string.length()));
  }
}
