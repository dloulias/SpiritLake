/*
 * Spirit Lake Software
 * Version 1.0
 * Demetra Loulias and Gabrielle Glynn
 * 
 * Takes readings at set alarm times from preferences CSV.
 * 
 * Still to be done:
 *  - Code clean-up
 *  - Alarm time sorting
 *  - Motor controls (with depth readings and multiple readings per depth)
 *  - Sleeping/Power-Saving
 */
 
typedef struct {
  int hour;
  int minute;
} AlarmTime;

#include <SPI.h>
#include <SD.h>
#include <ctype.h>
#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
#include <SDI12.h>

#define DATAPIN 10
#define CHIPSELECT 53
#define ALARM_PIN 2
#define MAX_READINGS 5


// Max number of readings a user can enter is 5.
AlarmTime alarms[MAX_READINGS];

int numAlarms;
int alarmCount = 0;
boolean wasCalled = false;

SDI12 mySDI12(DATAPIN);

void setup() {

  // Begin serial display.
  Serial.begin(115200);
  Serial.println("Setting up...");

  // Wait for SD card to be inserted.
  while(!SD.begin(CHIPSELECT)){
    Serial.println("Card failed.");
  }
  
  Serial.println("Card loaded.");
  
  Serial.println("Loading preferences...");
  
  // Read in csv file
  if(SD.exists("config.csv")){
    Serial.println("Preferences file found.");
    if(!getTimes()) {
      setDefaultAlarms();  
      Serial.println("Default times loaded.");
    }
    else
      Serial.println("Times loaded.");
  }
  else{
    setDefaultAlarms();
    Serial.println("Preferences file not found.");
    Serial.println("Default times loaded");
  }
  Serial.print("Current time: ");
  time_t currtime = RTC.get();
  Serial.print(month(currtime));
  Serial.print("/");
  Serial.print(day(currtime));
  Serial.print("/");
  Serial.print(year(currtime));
  Serial.print(" ");
  Serial.print(hour(currtime));
  Serial.print(":");
  Serial.print(minute(currtime));
  Serial.print(":");
  Serial.println(second(currtime));
  
  Serial.println("Setting up first alarm.");
  setSyncProvider(RTC.get);
  RTC.squareWave(SQWAVE_NONE);
  pinMode(ALARM_PIN, INPUT_PULLUP);
  attachInterrupt(INT0, alarmISR, FALLING);

  delay(5000);
  setAlarmTime();
  
}


void setDefaultAlarms() {
  alarms[0].hour = 0;
  alarms[0].minute = 0;
  
  alarms[1].hour = 4;
  alarms[1].minute = 0;
  
  alarms[2].hour = 4;
  alarms[2].minute = 0;
  
  alarms[3].hour = 4;
  alarms[3].minute = 0;
  
  alarms[4].hour = 4;
  alarms[4].minute = 0;
  
  alarms[5].hour = 4;
  alarms[5].minute = 0;
  
  numAlarms = 5;
  
}


bool getTimes() {
  
  File timeFile = SD.open("config.csv", FILE_READ);
  int alarmCounter = 0;
  char c;
  String timeAdd = "";
  bool returnBool = true;

  

  while(timeFile.available() && alarmCounter < MAX_READINGS) {
    
    c = timeFile.read();
    Serial.println(c);
    if(c == ':') {
      alarms[alarmCounter].hour = timeAdd.toInt();
      timeAdd = "";
    }
    else if(isdigit(c)){
      timeAdd += c;
    }
    else if(c == '\n') {
      alarms[alarmCounter].minute = timeAdd.toInt();
      timeAdd = "";
      alarmCounter++;
    }
    else {
      if(c != '\r') {
        returnBool = false;
        break;
      }
    }

  }

  if(isdigit(c)) {
    alarms[alarmCounter].minute = timeAdd.toInt();
    timeAdd = "";
    alarmCounter++;
  }
  else {
    if(c != '\r' || c != '\n') {
      returnBool = false;
    }
  }
  
  numAlarms = alarmCounter;

  for(int i = 0; i < numAlarms; i++) {
    Serial.print(alarms[i].hour);
    Serial.print(':');
    Serial.println(alarms[i].minute);
  }
  return returnBool;
  
}

void setAlarmTime() {
  Serial.print("Setting up alarm for ");
  Serial.print(alarms[alarmCount].hour);
  Serial.print(":");
  Serial.println(alarms[alarmCount].minute);
  RTC.setAlarm(ALM1_MATCH_HOURS, 0, alarms[alarmCount].minute, alarms[alarmCount].hour, 1);
  //RTC.setAlarm(ALM1_MATCH_HOURS, 0, 57, 13, 1);
  RTC.alarm(ALARM_1);
  RTC.alarmInterrupt(ALARM_1, true);
}

void alarmISR() {
  wasCalled = true;  
}

void takeMeasurement()
{
    while(!SD.begin(CHIPSELECT)){
      Serial.println("Card failed.");
    }
    turnOnSensor();
    requestMeasurement();
    String data = requestData();
    recordData(data);
    turnOffSensor();
}

String parseData(String data) {
  String toAdd = "";
  String line = "";
  Serial.println("Writing to SD card...");
  
  if(data.charAt(1) == '-')
    line += "-";

  for(int i = 2; i < data.length(); i++){
    
    char c = data.charAt(i);
    
    if(c == '+') {
      line += toAdd;
      line += ",";
      toAdd = "";
    }
    else if(i == data.length() - 1) {
      toAdd += c;
      line += toAdd;
      toAdd = "";
    }
    else if(c == 'N') {
      i++;
    }
    else {
      toAdd += c;
    }
       
  }

  return line;
  
}
void recordData(String data) {
  String line = parseData(data);
  File logfile;
  char filename[] = "logger.csv";
  
  
  
  if(!SD.exists(filename)){
    logfile = SD.open(filename, FILE_WRITE);
    logfile.println("Temperature,pH,Specific Conductance,Salinity,DO %Saturation,DO mg/L,ORP,Depth,Battery");
  }
  else {
    logfile = SD.open(filename, FILE_WRITE);
  }

  if(!logfile) {
    Serial.print("Couldn't create file");
  }
  else {
    logfile.println(line);
    logfile.close(); 
  }
}

String requestData() {
  Serial.println("Requesting data...");
  bool responded = false;
  String response = "";
  String returnResponse = "";

  while(!responded) {
    mySDI12.sendCommand("0D0!");
    delay(30);
    response = decodeResponse();
    Serial.println(response);
    if(!response.equals("0+0.00+0.00+0.000+0.00+0.0") && !response.equals(""))
    {
      responded = true;
    } 
    else if (response.equals("0")){
      turnOnSensor();
      requestMeasurement();
    }
  }

  responded = false;
  returnResponse = response;
  response = "";

  while(!responded) {
    mySDI12.sendCommand("0D1!");
    delay(30);
    response = decodeResponse();
    Serial.println(response);
    if(!response.equals("0+0.00+0+0.000+0.0") && !response.equals(""))
    {
      responded = true;
    }
  }

  returnResponse += "N";
  returnResponse += response;
  return returnResponse;
}
void requestMeasurement() {
  Serial.println("Requesting measurement...");
  bool responded = false;
  String response = "";
  
  while(!responded) {
    mySDI12.sendCommand("0M!");
    delay(30);

    response = decodeResponse();
    Serial.println(response);
    if(response.length() > 0)
    {
      responded = true;
    }
  }

  String waitFor = "";
  for(int i = 1; i < 4; i++) {
    waitFor += response.charAt(i);
  }

  unsigned int timed = waitFor.toInt();
  timed = timed * 1000;
  delay(timed);
}

void turnOnSensor() {
  Serial.println("Turning on sensor...");
  bool responded = false;
  while(!responded) {
    mySDI12.sendCommand("0X1!");
    delay(30);

    String response = decodeResponse();
    if(response.equals("0X1")) {
      responded = true;
    }
  }

  Serial.println("Sensor on!");
}

void turnOffSensor() {
  Serial.println("Turning off sensor...");
  bool responded = false;
  while(!responded) {
    mySDI12.sendCommand("0X0!");
    delay(30);

    String response = decodeResponse();
    if(response.equals("0X0")) {
      responded = true;
    }
  }

  Serial.println("Sensor off!");
}

String decodeResponse()
{
  String sdiResponse = "";
  while(mySDI12.available()) {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(5);
    }
  }
  
    mySDI12.flush();
    return sdiResponse;
}

void loop() {
  if(wasCalled) {
    Serial.print("Alarm!");

    if(alarmCount == numAlarms-1){
      alarmCount = 0;
    }
    else {
      alarmCount++;
    }
    setAlarmTime();

    wasCalled = false;
  }

}
