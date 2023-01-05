// Copyright (C) Wayne Geiser, 2018-2021.  All Rights Rreserved.
// email: geiserw@gmail.com
//
// Logs temperature and humidity data from a DHT22 connect to an Arduino to a ThingSpeak channel via WiFi
//
// NOTE **************
// Remove the sheild before uploading!!!!
//
// Dependencies:
//    DHT sensor library
//    watchdog timer library
//
// Change log:
//  2018-Oct-29 Changed channel on ThingSpeak to separate this data from other data being logged to the previous channel
//              Added char definitions for Serial calls to remove warnings upon compilation
//  2019-Feb-26 Changed WiFi code to loop continuously trying to connect so it might recover from a power outage.
//  2021-Mar-06 Added Watchdog code in case of hang.
//  2023-Jan-02 Wait around for the WiFi network to connect
//              Rest the arduino if the data can't be updated.  It probably means we lost WiFi without losing power.

#include <stdlib.h>
#include "DHT.h"
#include "SecretStuff.h" // definitions for SSID and PASS (your network name and password)
#include <avr/wdt.h>

// Configuration needed to get this to run in your environment is surrounded by <! !>
#define IP "184.106.153.149" // thingspeak.com
#define DHTPIN 7     // what pin the DHT sensor is connected to
#define DHTTYPE DHT22   // Change to DHT11 if that's what you have
#define Baud_Rate 115200 //Another common value is 9600
#define GREEN_LED 3 //optional LED's for debugging
#define RED_LED 4 //optional LED's for debugging
#define RESET 6 // Write to this pin will reset the Arduino
#define DELAY_TIME 600000 // 10 mins

//Can use a post also
String GET = "GET /update?key=T3459S54GG8YFXBU&field1="; // Basement Data channel
String FIELD2 = "&field2=";

//if you want to add more fields this is how
//String FIELD3 = "&field3=";

bool updated;

DHT dht(DHTPIN, DHTTYPE);

//this runs once
void setup() {
  // Watchdog timer
  wdt_enable(WDTO_8S);
  
  char OK[] = "OK";

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  LEDOn(RED_LED);
  Serial.begin(Baud_Rate);
  // ESP32 module connected and functioning?
  Serial.println("AT");
  
  watchdog_delay(10000);
  
  if (Serial.find(OK)) {
    //connect to your wifi netowork
    bool connected = connectWiFi();
    if (!connected) {
      //failure, need to check your values and try again
      // Note: we should NEVER get here.  connectWifi should wait around for the connection
      Error(1);
    }
  }
  else {
    Error(2);
  }
 
  //initalize DHT sensor
  dht.begin();
  LEDOff(GREEN_LED);
}

//this runs over and over
void loop() {
  float h = dht.readHumidity();
   // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    LightRed();
    watchdog_delay(1000);
    LightGreen();
    return;
  }

  //update ThingSpeak channel with new values
  updated = updateTemp(String(f), String(h));
  
  //if update succeeded light up green LED, else light up red LED
  if (updated) {
    LightGreen();
  }
  else {
    LightRed();      
    digitalWrite(RESET, LOW); // reset the device, we probably lost WiFi
  }
  watchdog_delay(1000);
  LEDOff(GREEN_LED);
  LEDOff(RED_LED);

  watchdog_delay(DELAY_TIME - 1000);

  // blink to show we're working
  LEDOff(GREEN_LED);
  LEDOff(RED_LED);
  watchdog_delay(1000);
}

void watchdog_delay(long delay_ms) {
  long num_trips = delay_ms / 500; // number of 1/2 second delays to do
  
  for (long i = 0; i < num_trips; i++) {
    wdt_reset();
    delay(500); // 0.5 sec
  }
}

bool updateTemp(String tenmpF, String humid) {
  char ERROR[] = "Error";
  char GREATER[] = ">";
  char OK[] = "OK";
  
  //initialize your AT command string
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  
  //add IP address and port
  cmd += IP;
  cmd += "\",80";
  
  //connect
  Serial.println(cmd);
  watchdog_delay(2000);
  if (Serial.find(ERROR)) {
    return false;
  }
  
  //build GET command, ThingSpeak takes Post or Get commands for updates, I use a Get
  cmd = GET;
  cmd += tenmpF;
  cmd += FIELD2;
  cmd += humid;
  
  //continue to add data here if you have more fields such as a light sensor
  //cmd += FIELD3;
  //cmd += <field 3 value>
  
  cmd += "\r\n";
  
  //Use AT commands to send data
  Serial.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  if (Serial.find(GREATER)) {
    //send through command to update values
    Serial.print(cmd);
  }
  else {
    Serial.println("AT+CIPCLOSE");
  }
  
  if (Serial.find(OK)) {
    //success! Your most recent values should be online.
    return true;
  }
  else {
    return false;
  }
}

void clearInputBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}
 
boolean connectWiFi() {
  char OK[] = "OK";

  //set ESP8266 mode with AT commands
  String cmd1 = "AT+RST";
  String cmd2 = "AT+CWMODE=1";
  String cmd3 = "AT+CWJAP=\"";
  cmd3 += SECRET_SSID;
  cmd3 += "\",\"";
  cmd3 += SECRET_PASS;
  cmd3 += "\"";
  
  while (true) {
    //connect to WiFi network and wait 5 seconds
    Serial.println(cmd1);
    watchdog_delay(1000);
    clearInputBuffer();
    Serial.println(cmd2);
    watchdog_delay(1000);
    clearInputBuffer();
    Serial.println(cmd3);
    watchdog_delay(5000);
  
    //if connected return true, else wait
    if (Serial.find(OK)) {
      wdt_reset(); // pet the watchdog
      blinkLED(GREEN_LED);
      return true;
    }
    else {
      blinkLED(RED_LED);      
    }
  }
}

void blinkLED(int led) {
  LEDOn(led);
  watchdog_delay(500);
  LEDOff(led);
  watchdog_delay(500);
}

void LEDOn(int led) {
  digitalWrite(led, HIGH);
}

void LEDOff(int led) {   
  digitalWrite(led, LOW);
}

void LightGreen() {
  LEDOff(RED_LED);
  LEDOn(GREEN_LED);  
}

void LightRed() {
  LEDOff(GREEN_LED);
  LEDOn(RED_LED);
}

//if an error has occurred alternate green and red leds
void Error(int code) {
  int i;
  int j = 0;

  while (j < 10) {
    i = code;
    do {    
      blinkLED(RED_LED);
      blinkLED(GREEN_LED);
      i--;
    } while (i > 0);
    LEDOff(GREEN_LED);
    LEDOff(RED_LED);
    watchdog_delay(4000);
    j++;
  }
  delay(100000); // Let it reboot
}
