/*
 * Spirit Lake Software
 * Version 1.1
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

// Struct to hold hour and minute of alarms
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

#define DATAPIN 10      // SDI-12 pin
#define CHIPSELECT 53   // Chip select for SD card
#define ALARM_PIN 2     // Interrupt pin from RTC
#define MAX_READINGS 5 

AlarmTime alarms[MAX_READINGS]; // Holds all reading times

int numAlarms;  // Total number of alarms that are set. (Max of 5)
int alarmCount = 0; // The current alarm the program is on.
boolean wasCalled = false; // Boolean value for whether or not the alarm has gone off.

SDI12 mySDI12(DATAPIN); // Attach SDI-12 communication to data pin.

void setup() {
  Serial.begin(115200); // Begin serial display
  mySDI12.begin();  // Begin SDI-12 communication
  
  Serial.println("Setting up...");

  // Wait for SD card to be inserted.
  while(!SD.begin(CHIPSELECT)){
    Serial.println("Card failed.");
  }
  
  Serial.println("Card loaded.");
  Serial.println("Loading preferences...");
  
  // Read in CSV file containing alarms
  if(SD.exists("config.csv")){
    Serial.println("Preferences file found.");
    if(!getTimes()) {
      setDefaultAlarms();  // Set to defaults if incorrect format.
      Serial.println("Default times loaded.");
    }
    else
      Serial.println("Times loaded.");
  }
  else{
    setDefaultAlarms(); // Set to defaults if file does not exist
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

  
  //setSyncProvider(RTC.get); // Synchronizes RTC with Time library (not necessary)
  
  // Disable default square wave of pin
  RTC.squareWave(SQWAVE_NONE);  
  
  // Attach interrupt to pin
  pinMode(ALARM_PIN, INPUT_PULLUP); 
  attachInterrupt(INT0, alarmISR, FALLING);

  // set first alarm
  setAlarmTime();
  
}

/**
 * Sets default alarm times if there is an issue with the config file.
 */
void setDefaultAlarms() {
  alarms[0].hour = 0;
  alarms[0].minute = 0;
  
  alarms[1].hour = 4;
  alarms[1].minute = 0;
  
  alarms[2].hour = 8;
  alarms[2].minute = 0;
  
  alarms[3].hour = 12;
  alarms[3].minute = 0;
  
  alarms[4].hour = 16;
  alarms[4].minute = 0;
  
  alarms[5].hour = 20;
  alarms[5].minute = 0;
  
  numAlarms = 5;
  
}

/**
 * Parses the config file on the SD card and stores the alarm times.
 * 
 * @return whether or not the file is the appropriate format.
 */
bool getTimes() {
  // open config file
  File timeFile = SD.open("config.csv", FILE_READ); 
  
  // counter for number of alarms
  int alarmCounter = 0; 
  // the character being looked at
  char c; 
  // the time to be added
  String timeAdd = ""; 
  // return boolean 
  bool returnBool = true; 
  
  while(timeFile.available() && alarmCounter < MAX_READINGS) {
    c = timeFile.read();

    // if colon, the previous numbers were hours
    if(c == ':') {
      alarms[alarmCounter].hour = timeAdd.toInt(); 
      timeAdd = "";
    }
    // if digit, add to string to be added
    else if(isdigit(c)){
      timeAdd += c; 
    }
    // if new line, add the remaining number
    else if(c == '\n') {
      alarms[alarmCounter].minute = timeAdd.toInt();
      timeAdd = "";
      alarmCounter++;
    }
    // if any other characters are contained in the file, it is not valid
    else {
      if(c != '\r') {
        returnBool = false;
        break;
      }
    }

  }

  // add the last character
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

  // set number of alarms to number of alarms counted
  numAlarms = alarmCounter;

  for(int i = 0; i < numAlarms; i++) {
    Serial.print(alarms[i].hour);
    Serial.print(':');
    Serial.println(alarms[i].minute);
  }
  
  return returnBool;
  
}

/**
 * Sets the alarm to the time held in alarmCount.
 */
void setAlarmTime() {
  Serial.print("Setting up alarm for ");
  Serial.print(alarms[alarmCount].hour);
  Serial.print(":");
  Serial.println(alarms[alarmCount].minute);

  // set Alarm 1 time
  RTC.setAlarm(ALM1_MATCH_HOURS, 0, alarms[alarmCount].minute, alarms[alarmCount].hour, 1);
  // clear RTC flag
  RTC.alarm(ALARM_1);
  // allow interrupt
  RTC.alarmInterrupt(ALARM_1, true);
}

/**
 * ISR for when Alarm1 goes off.
 */
void alarmISR() {
  wasCalled = true;  
}

/**
 * Takes measurement from the sensor and stores it to an SD card.
 */
void takeMeasurement()
{
    turnOnSensor();
    requestMeasurement();
    String data = requestData();
    recordData(data);
    turnOffSensor();
}

/**
 * Parses the string of data given by the sensor.
 * 
 * @param the data to be parsed.
 * @return the CSV line to be added to the logger file.
 */
String parseData(String data) {
  // the line to be added to
  String line = "";

  // add timestamp to the line
  time_t currtime = RTC.get();
  line += month(currtime);
  line += "/";
  line += day(currtime);
  line += "/";
  line += year(currtime);
  line += ",";
  line += hour(currtime);
  line += ":";
  if(minute(currtime) < 10)
    line += "0";
  line += minute(currtime);
  line += ",";

  // the string to be added to the line
  String toAdd = "";
  char c;
  
  Serial.println("Writing to SD card...");

  // check for negative sign of the first reading
  if(data.charAt(1) == '-')
    line += "-";

  // loop through characters in data;
  for(int i = 2; i < data.length(); i++){
    
    c = data.charAt(i);
    
    if(c == '+') {
      line += toAdd;
      line += ",";
      toAdd = "";
    }
    else if(c == '-') {
      line += toAdd;
      line += ",-";
      toAdd = "";
    }
    else if(i == data.length() - 1) {
      toAdd += c;
      line += toAdd;
      toAdd = "";
    }
    // N is a character added between the 2 data readings from the sensor to differentiate
    // the two bits of data.
    else if(c == 'N') {
      i++;
    }
    else {
      toAdd += c;
    }
       
  }

  return line;
}

/**
 * Adds the given line of data to the logger CSV file.
 * 
 * @param the data to be added.
 */
void recordData(String data) {
  // parse the data and make it into the formatted line to be added
  String line = parseData(data);
  // the logger file to be written to;
  File logfile;
  // the filename of the logger file
  char filename[] = "logger.csv";

  // if it doesn't exist, create the file and it's header
  if(!SD.exists(filename)){
    logfile = SD.open(filename, FILE_WRITE);
    logfile.println("Date,Time,Temperature,pH,Specific Conductance,Salinity,DO %Saturation,DO mg/L,ORP,Depth,Battery");
  }
  // if it exists, open the file
  else {
    logfile = SD.open(filename, FILE_WRITE);
  }

  if(!logfile) {
    Serial.print("Couldn't create file");
  }
  else {
    // add line to file and close the file
    logfile.println(line);
    logfile.close(); 
  }
}

/**
 * Requests the measured data from the sensor.
 * 
 * @return the String of data.
 */
String requestData() {
  Serial.println("Requesting data...");

  // boolean to wait for response
  bool responded = false;
  // the sensor response
  String response = "";
  // the response to be returned
  String returnResponse = "";

  // get second line of data
  // wait for valid response 
  while(!responded) {
    // send "report data" command for first line of data
    mySDI12.sendCommand("0D0!");
    // wait a bit
    delay(30);
    // get response
    response = decodeResponse();
    
    Serial.println(response);

    // valid response
    if(!response.equals("0+0.00+0.00+0.000+0.00+0.0") && !response.equals(""))
    {
      responded = true;
    } 
    // a response of '0' means that the sensor needs to be restarted
    else if (response.equals("0")){
      turnOnSensor();
      requestMeasurement();
    }
  }

  // add response to return string
  returnResponse = response;
  
  // reset for second line of data
  responded = false;
  response = "";

  // get second line of data
  // wait for valid response
  while(!responded) {
    // send "report data" command for second line of data
    mySDI12.sendCommand("0D1!");
    // wait a bit
    delay(30);
    // get response
    response = decodeResponse();
    
    Serial.println(response);

    // valid response
    if(!response.equals("0+0.00+0+0.000+0.0") && !response.equals(""))
    {
      responded = true;
    }
  }

  // add 'N' in between first and second lines to make parsing easier
  returnResponse += "N";
  // add second response to return string
  returnResponse += response;
  
  return returnResponse;
}

/**
 * Commands the sensor to take a measurement.
 */
void requestMeasurement() {
  Serial.println("Requesting measurement...");

  // boolean to wait for response
  bool responded = false;
  // string for response
  String response = "";

  // wait for valid response
  while(!responded) {
    // send measurement command
    mySDI12.sendCommand("0M!");
    // wait a bit
    delay(30);
    // get response
    response = decodeResponse();
    
    Serial.println(response);

    // if response contains characters, the sensor responded
    if(response.length() > 0)
    {
      responded = true;
    }
  }

  // parse response to figure out how long to wait
  String waitFor = "";
  for(int i = 1; i < 4; i++) {
    waitFor += response.charAt(i);
  }
  unsigned int timed = waitFor.toInt();

  // delay for a period of time
  timed = timed * 1000;
  delay(timed);
}

/**
 * Turns on the sensor.
 */
void turnOnSensor() {
  Serial.println("Turning on sensor...");
  // boolean to wait for response
  bool responded = false;

  // wait for valid response
  while(!responded) {
    // send sensors on command
    mySDI12.sendCommand("0X1!");
    // wait a bit
    delay(30);
    // get response
    String response = decodeResponse();
    // check if response is valid
    if(response.equals("0X1")) {
      responded = true;
    }
  }

  Serial.println("Sensor on!");
}

/**
 * Turns off sensor.
 * Note: I have tried making the on/off functions the same, and for some reason the 
 * sensor doesn't like it.
 */
void turnOffSensor() {
  Serial.println("Turning off sensor...");
  // boolean to wait for response
  bool responded = false;

  // wait for valid response
  while(!responded) {
    // send sensors off command
    mySDI12.sendCommand("0X0!");
    // wait a bit
    delay(30);
    // get response
    String response = decodeResponse();
    // check if response is valid
    if(response.equals("0X0")) {
      responded = true;
    }
  }

  Serial.println("Sensor off!");
}

/**
 * Gets the response from the sensor by getting all of the available bits from
 * the data line.
 * 
 * @return the response of the sensor.
 */
String decodeResponse() {
  // string to hold response
  String sdiResponse = "";

  // loop through available data bits
  while(mySDI12.available()) {
    // read character
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(5);
    }
  }

  // flush the data line
  mySDI12.flush();
  
  return sdiResponse;
}

void loop() {
  // check to see if alarm has gone off
  if(wasCalled) {
    Serial.print("Alarm!");

    // set to next alarm
    if(alarmCount == numAlarms-1){
      alarmCount = 0;
    }
    else {
      alarmCount++;
    }

    // clear alarm flag
    RTC.alarm(ALARM_1);

    // take measurement from sensor
    takeMeasurement();

    // clear boolean
    wasCalled = false;
    
    // set to next alarm
    setAlarmTime();
  }

}
