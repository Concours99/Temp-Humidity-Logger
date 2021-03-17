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

#include<stdlib.h>
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
#define DELAY_TIME 600000 // 10 mins

//Can use a post also
String GET = "GET /update?key=T3459S54GG8YFXBU&field1="; // Basement Data channel
String FIELD2 = "&field2=";

//if you want to add more fields this is how
//String FIELD3 = "&field3=";

bool updated;

DHT dht(DHTPIN, DHTTYPE);

//this runs once
void setup()
{
  char OK[] = "OK";
  
  wdt_enable(WDTO_8S);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  Serial.begin(Baud_Rate);
  Serial.println("AT");
  
  watchdog_delay(10000);
  
  if(Serial.find(OK)){
    //connect to your wifi netowork
    bool connected = connectWiFi();
    if(!connected){
      //failure, need to check your values and try again
      Error(1);
    }
  }else{
    Error(2);
  }
  
  //initalize DHT sensor
  dht.begin();
}

//this runs over and over
void loop(){
  float h = dht.readHumidity();
   // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    LightRed();
    return;
  }

  //update ThingSpeak channel with new values
  updated = updateTemp(String(f), String(h));
  
  //if update succeeded light up green LED, else light up green and red LED (to differentiate
  // from the LightRed call (above)
  if(updated){
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  }else{
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
  }

  watchdog_delay(DELAY_TIME);

  // blink to show we're working
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  watchdog_delay(1000);
}

void watchdog_delay(long delay_ms) {
  long num_trips = delay_ms / 500; // number of 1/2 second delays to do
  
  for (long i = 0; i < num_trips; i++) {
    wdt_reset();
    delay(500); // 0.5 sec
  }
}

bool updateTemp(String tenmpF, String humid){
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
  if(Serial.find(ERROR)){
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
  if(Serial.find(GREATER)){
    //send through command to update values
    Serial.print(cmd);
  }else{
    Serial.println("AT+CIPCLOSE");
  }
  
  if(Serial.find(OK)){
    //success! Your most recent values should be online.
    return true;
  }else{
    return false;
  }
}
 
boolean connectWiFi(){
  char OK[] = "OK";

  while (true) { // we have nothing else to do, so keep trying
    //set ESP8266 mode with AT commands
    Serial.println("AT+CWMODE=1");
    watchdog_delay(2000);

    //build connection command
    String cmd="AT+CWJAP=\"";
    cmd+=SECRET_SSID;
    cmd+="\",\"";
    cmd+=SECRET_PASS;
    cmd+="\"";
  
    //connect to WiFi network and wait 5 seconds
    Serial.println(cmd);
    watchdog_delay(5000);
  
    //if connected return true, else false
    if(Serial.find(OK)){
      return true;
    }
    else {
      watchdog_delay(60000); // wait a minute
      //return false;
    }
  }
}

void LightGreen(){
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);  
}

void LightRed(){
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
}

//if an error has occurred alternate green and red leds
void Error(int code){
  int i;
  int j = 0;

  while (j < 10) {
    i = code;
    do {    
      LightRed();      
      watchdog_delay(2000);      
      LightGreen();
      watchdog_delay(2000);
      i--;
    } while (i > 0);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    watchdog_delay(4000);
    j++;
  }
  delay(100000); // Let it reeboot
}
