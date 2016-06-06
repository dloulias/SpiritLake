/*
 * Spirit Lake Software
 * Version 1.4
 * Demetra Loulias and Gabrielle Glynn
 * 
 * Takes readings at set alarm times from preferences CSV.
 * 
 * Still to be done:
 *  - Get depth when program is started
 *  - Make sure that the sensor is actually going down
 *  - Match depth reading to varible during readings
 */

// Struct to hold hour and minute of alarms
typedef struct {
  int hour;
  int minute;
} AlarmTime;

#include <LowPower.h>
#include <SPI.h>
#include <SD.h>
#include <ctype.h>
#include <DS3232RTC.h>        //http://github.com/JChristensen/DS3232RTC
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <Wire.h>             //http://arduino.cc/en/Reference/Wire
#include <SDI12.h>
#include "LiquidCrystal.h"

#define D4 4 // LCD D4
#define D5 5 // LCD D5
#define D6 6 // LCD D6
#define D7 7 // LCD D7
#define RS 8 // LCD RS
#define E 9 // LCD E
#define DATAPIN 10      // SDI-12 pin
#define ALARM_PIN 18     // Interrupt pin from RTC
#define UP 12   // Up pin (for transistor)
#define DOWN 13 // Down pin (for transistor)
#define LCDON 14 // Turns on transistors for LCD
#define TIMEUP 15 // Button for time up
#define TIMEDOWN 16 // Button for time down
#define PROGRAM 17 // Button for program
#define CHIPSELECT 53   // Chip select for SD card
#define MAX_READINGS 10 
#define MAX_DEPTH 460 // The maximum depth the sensor will go, in meters

AlarmTime alarms[MAX_READINGS]; // Holds all reading times

int numAlarms;  // Total number of alarms that are set. (Max of 5)
int alarmCount; // The current alarm the program is on.
float lowest;  // The lowest point in the water the sensor goes.
float highest; // The highest point in the water the sensor goes.
boolean motorOn; // Boolean value for whether or not the motor should be used
boolean goUp;   // Boolean for whether the motor should take readings going up or down

SDI12 mySDI12(DATAPIN); // Attach SDI-12 communication to data pin.
LiquidCrystal lcd(RS,E,D4,D5,D6,D7);

void setup() {
  Serial.begin(115200); // Begin serial display
  mySDI12.begin();  // Begin SDI-12 communication
  
  pinMode(UP, OUTPUT);
  pinMode(DOWN, OUTPUT);
  pinMode(TIMEUP, INPUT_PULLUP);
  digitalWrite(TIMEUP, HIGH);
  pinMode(TIMEDOWN, INPUT_PULLUP);
  digitalWrite(TIMEDOWN, HIGH);
  pinMode(PROGRAM, INPUT_PULLUP);
  digitalWrite(PROGRAM, HIGH);
  pinMode(LCDON, OUTPUT);

  digitalWrite(LCDON, HIGH);
  lcd.begin(16,2);

    Serial.println("Setting up...");

  // Wait for SD card to be inserted.
  while(!SD.begin(CHIPSELECT)){
    lcdPrintMessage("    CARD NOT", "    INSERTED ");
  }

  lcdPrintMessage(" Card Inserted", "");
  delay(1000);
  Serial.println("Card loaded.");

  time_t currtime = RTC.get();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Current Time:");
  lcd.setCursor(0,1);
  lcd.print(month(currtime));
  lcd.print("/");
  lcd.print(day(currtime));
  lcd.print("/");
  lcd.print(year(currtime));
  lcd.print(" ");
  lcd.print(hour(currtime));
  lcd.print(":");
  lcd.print(minute(currtime));
  delay(3000);
  
  int counter = 10;
  
  while(true){
    if(digitalRead(PROGRAM) == LOW){
      programClock();
      programDepths();
      break;  
    }
    if(counter == -1) {
      break;
    }

    lcdPrintMessage("Starting in...", String(counter));
    counter--;
    delay(1000);
  }
  
  
  lcdPrintMessage("    Loading", " preferences...");
  delay(1000);
  Serial.println("Loading preferences...");
  
  // Read in CSV file containing alarms
  if(SD.exists("config.csv")){
    if(!getTimes()) {
      setDefaultAlarms();  // Set to defaults if incorrect format.
      Serial.println("Default times loaded.");
      lcdPrintMessage("Preferences file", "   not found.");
      delay(2000);
      lcdPrintMessage(" Default times", "    loaded.");
    }
    else
      lcdPrintMessage("Preferences file", "     found.");
  }
  else{
    setDefaultAlarms(); // Set to defaults if file does not exist
    lcdPrintMessage("Preferences file", "   not found.");
    delay(2000);
    lcdPrintMessage(" Default times", "    loaded.");
  }

  delay(2000);

  lcdPrintMessage("Loading", " depths...");
  delay(2000);
  if(getHeights()){
    Serial.println("Moving motor to starting position...");
    lcdPrintMessage("Moving to start", "  position...");
    bool startReached = false;
    
    int timeDelay = 1000;
    float diff;
    float prevDiff;
    float depth;
    float prevDepth = 0;
    
    while(true) {
      depth = takeMeasurement(true);

      prevDiff = abs(depth - prevDepth);
      diff = lowest - depth;

      if(prevDepth != 0) {
        if(prevDiff < 0.05 || prevDiff == 0.05) {
          Serial.println("Motor off!");
          motorOn = false;
          lcdPrintMessage("Motor error", "");
          delay(2000);
          lcdPrintMessage("Operating","without motor.");
          delay(2000);
          break;
        }
      }
      else if(diff < 0.1) {
        goUp = true;
        motorOn = true;
        break;
      }
      else if(diff < 0.2) {
        timeDelay = 500;
      }
      else if(diff < 0 || diff == 0) {
          motorOn = false;  
      }
      

      prevDepth = depth;
      digitalWrite(DOWN, HIGH);
      delay(timeDelay);
      digitalWrite(DOWN, LOW);
    }  
  }
  else {
    motorOn = false;
  }
  


  Serial.print("Current time: ");
  currtime = RTC.get();
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
  
  // Set pinmode to input
  pinMode(ALARM_PIN, INPUT); 
  lcdPrintMessage("Ready to", "start!");
  digitalWrite(LCDON, LOW);
  // set first alarm
  setAlarmTime();

  delay(5000);
  
}

void programDepths() {
  float depthHighest;
  float depthLowest;
  File depthFile;
  
  lcdPrintMessage("Move sensor to", "highest position.");
  delay(2000);
  while(true) {
    if(digitalRead(PROGRAM) == LOW){
      lcdPrintMessage("Saving...","");
      float depth1 = takeMeasurement(true);
      float depth2 = takeMeasurement(true);
      float depth3 = takeMeasurement(true);
      
      depthHighest = (depth1 + depth2 + depth3)/3;
      Serial.print("Calculated Value:");
      Serial.println(depthHighest);
      break;
    }
  }

  lcdPrintMessage("Move sensor to", "lowest position.");
  delay(2000);
  while(true) {
    if(digitalRead(PROGRAM) == LOW){
      lcdPrintMessage("Saving...","");
      float depth4 = takeMeasurement(true);
      float depth5 = takeMeasurement(true);
      float depth6 = takeMeasurement(true);

      depthLowest = (depth4 + depth5 + depth6)/3;
      Serial.print("Calculated Value:");
      Serial.println(depthLowest);
      break;
    }
  }

  if(depthHighest > depthLowest) {
    float temp = depthHighest;
    depthHighest = depthLowest;
    depthLowest = temp;
  }

  if(SD.exists("range.txt")) {
    Serial.println("EXISTS");
    SD.remove("range.txt");
  }

  File myFile = SD.open("range.txt", FILE_WRITE);
  if(myFile){
    myFile.println(depthHighest);
    myFile.println(depthLowest);
    myFile.close();
  } 
  else {
    Serial.println("error");
    lcdPrintMessage("error","");
  }
  
  
  delay(2000);
}

boolean getHeights() {
  File depthFile = SD.open("range.txt");
  int lineNum = 1;
  String line = "";
  float depth;
  
  if(depthFile) {
    Serial.println("FOUND!");
    while(depthFile.available() != 0) {
      Serial.println("reading...");
      line = depthFile.readStringUntil('\n');
      depth = line.toFloat(); // returns 0 if not actually a float
      Serial.println(line);
      if(depth == 0) {
        lcdPrintMessage("Incorrect", "format.");
        delay(2000);
        return false;   
      }
      else if(lineNum == 1) {
        highest = depth;
        lineNum++;
      }
      else if(lineNum == 2) {
        lowest = depth;
        lineNum++;
      }
      else {
        lcdPrintMessage("Incorrect", "format.");
        delay(2000);
        return false;
      }
    }
    Serial.println("Linenum");
    Serial.println(lineNum);
    if(lineNum < 3){
      Serial.println("Incorrect format");
      lcdPrintMessage("Incorrect", "format.");
      delay(2000);
      return false;
    }
    else
      return true;
  } 
  else{
    Serial.println("NOT FOUND!");
    lcdPrintMessage("Depth file", "not found.");
    delay(2000);
    return false;
  }
}
void lcdPrintMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

void programClock() {
  digitalWrite(LCDON, HIGH);
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Programming");
  lcd.setCursor(0,1);
  lcd.print("Mode");
  
  Serial.println("Programming clock:");
  Serial.println("Current time:");
  
  delay(1500);


  tmElements_t tm;
  time_t timeSet;
  
  time_t currtime = RTC.get();
  int currMonth = month(currtime);
  int currDay = day(currtime);
  int currYear = year(currtime);
  int currHour = hour(currtime);
  int currMinute = minute(currtime);

  delay(2000);
  
  Serial.print(currMonth);
  Serial.print("/");
  Serial.print(currDay);
  Serial.print("/");
  Serial.print(currYear);
  Serial.print(" ");
  Serial.print(currHour);
  Serial.print(":");
  Serial.println(currMinute);

  delay(300);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set month:");
  lcd.setCursor(0,1);
  
  currMonth = getEntry(12, 1, currMonth, "Set Month:");
  
  Serial.println("Set day:");
  currDay = getEntry(31, 1, currDay, "Set Day:");

  Serial.println("Set year");
  currYear = getEntry(2030, 2016, currYear, "Set year:");

  Serial.println("Set hour:");
  currHour = getEntry(23, 0, currHour, "Set Hour:");

  Serial.println("Set minute:");
  currMinute = getEntry(59, 0, currMinute, "Set Minute");

  tm.Month = currMonth;
  tm.Day = currDay;
  tm.Year = CalendarYrToTm(currYear);
  tm.Hour = currHour;
  tm.Minute = currMinute;
  tm.Second = 0;

  timeSet = makeTime(tm);
  RTC.set(timeSet);
  Serial.println("Time set!");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Time set!");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Current Time:");
  lcd.setCursor(0,1);
  lcd.print(month(timeSet));
  lcd.print("/");
  lcd.print(day(timeSet));
  lcd.print("/");
  lcd.print(year(timeSet));
  lcd.print(" ");
  lcd.print(hour(timeSet));
  lcd.print(":");
  lcd.print(minute(timeSet));
  delay(3000);
}

int getEntry(int high, int low, int initialVal, String prompt) {
  int returnVal = initialVal;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(prompt);
  lcd.setCursor(0,1);
  lcd.print(returnVal);
  
  while(true) {
    if(digitalRead(TIMEUP) == LOW) {
      if(returnVal < high) {
        returnVal++;      
      }
      else {
        returnVal = low;
      }
      Serial.println(returnVal);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(prompt);
      lcd.setCursor(0,1);
      lcd.print(returnVal);
      delay(500);
    }
    if(digitalRead(TIMEDOWN) == LOW) {
      if(returnVal > low) {
        returnVal--;      
      }
      else {
        returnVal = high;
      }
      Serial.println(returnVal);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(prompt);
      lcd.setCursor(0,1);
      lcd.print(returnVal);
      delay(500);
    }
    if(digitalRead(PROGRAM) == LOW) {
      delay(300);
      Serial.println(returnVal);
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(prompt);
      lcd.setCursor(0,1);
      lcd.print(returnVal);
      
      break;
    } 
  }
  
  return returnVal;
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
  // buffer for sorting
  AlarmTime buffer;
  // counters
  byte i,j;
  
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
              Serial.println("FAILED1");
      Serial.println(c, DEC);
      Serial.println(c);
      Serial.println("FAILED2");
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
    if(c != '\r' && c != '\n' && c != 10) {
      Serial.println("FAILED1");
      Serial.println(c, DEC);
      Serial.println(c);
      Serial.println("FAILED2");
      returnBool = false;
    }
  }

  // set number of alarms to number of alarms counted
  numAlarms = alarmCounter;
  
  Serial.println(numAlarms);
  Serial.println("Sorting...");
  // organizes array into ascending order
  for (i=0; i<(alarmCounter); i++) {
    for (j=i+1; j<(alarmCounter); j++) {  
      if (alarms[i].hour > alarms[j].hour) {
        buffer = alarms[i];
        alarms[i] = alarms[j];
        alarms[j] = buffer;
      } else if (alarms[i].hour == alarms[j].hour) {
        if (alarms[i].minute > alarms[j].minute) {
          buffer = alarms[i];
          alarms[i] = alarms[j];
          alarms[j] = buffer;
        } else if (alarms[i].minute == alarms[j].minute) {
          //need to add code to get rid of previous equal
          //maybe set to 99:99
          alarms[j].hour = 25;
          alarms[j].minute = 25;
          numAlarms--;
        }
      }
    }
  }

  // finds soonest alarm time to set
  time_t currtime = RTC.get();
  int currHour = hour(currtime);
  int currMinute = minute(currtime);
  int nearAlarm;
 
  for(i = 0; i < numAlarms; i++) {
    Serial.println(i);
    if(alarms[i].hour > currHour || (alarms[i].hour == currHour && alarms[i].minute > currMinute)) {
      nearAlarm = i;
      break;
    }
  }
  
  Serial.println("NearAlarm");
  Serial.println(nearAlarm);

  alarmCount = nearAlarm;
  
  for(i = 0; i < numAlarms; i++) {
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
  int minuteAlarm = alarms[alarmCount].minute;
  int hourAlarm = alarms[alarmCount].hour;
  Serial.print("Setting up alarm for ");
  Serial.print(alarms[alarmCount].hour);
  Serial.print(":");
  Serial.println(alarms[alarmCount].minute);

  // set Alarm 1 time
  RTC.setAlarm(ALM1_MATCH_HOURS, 0, minuteAlarm, hourAlarm, 1);
  //RTC.setAlarm(ALM1_MATCH_HOURS, 0, 59, 15, 1);
  // clear RTC flag
  RTC.alarm(ALARM_1);
  // allow interrupt
  RTC.alarmInterrupt(ALARM_1, true);
}

/**
 * ISR for when Alarm1 goes off.
 */
void alarmISR() {
  
}

void takeMeasurementWithMotor() {
  int timeDelay;
  float depth;
  float prevDepth = 0;
  float diff;
  float prevDiff;
  
  if(goUp) {
    Serial.println("Going up!");
    timeDelay = 1500;
    while(true) {
      depth = takeMeasurement(false);
      diff = depth - highest;
      prevDiff = abs(prevDepth - depth);
      
      Serial.print("Depth:");
      Serial.println(depth);
      Serial.print("Diff:");
      Serial.println(diff);

      if(prevDepth != 0) {
        if(prevDepth < 0.05 || prevDiff == 0.05) {
          motorOn = false;
          break;
        }
      }
      else if(diff < 0.1) {
        Serial.println("Top reached!");
        goUp = false;
        break;  
      }
      else if(diff < 0.2) {
        Serial.println("Top not reached, try again!");
        timeDelay = 700;
      }
      else if(diff < 0 || diff == 0) {
        motorOn = false;  
        break;
      }

      prevDepth = depth;
      
      digitalWrite(UP, HIGH);
      delay(timeDelay);
      digitalWrite(UP, LOW); 
    }
  }
  else {
    Serial.println("Going down!");
    timeDelay = 700;
    while(true) {
      depth = takeMeasurement(false);
      diff = lowest - depth;
      prevDiff = abs(depth - prevDepth);

      if(prevDepth != 0) {
        if(prevDiff < 0.05 || prevDiff == 0.05) {
          motorOn = false;
          break;
        }
      }
      if(diff < 0.1) {
        Serial.println("Bottom reached!");
        goUp = true;
        break;
      }
      else if(diff < 0.2) {
        Serial.println("Bottom not reached, try again");
        timeDelay = 500;
      }
      else if(diff < 0 || diff == 0) {
        motorOn = false;  
      }

      digitalWrite(DOWN, HIGH);
      delay(timeDelay);
      digitalWrite(DOWN, LOW); 
    }
  }
}

/**
 * Takes measurement from the sensor and stores it to an SD card.
 */
float takeMeasurement(bool setUp) {
    turnOnSensor();
    requestMeasurement();
    String data = requestData();
    float depth = 0;
    
    if(!setUp)  {
      recordData(data);
      
    }

    depth = getDepth(data); 
    
    turnOffSensor();

    return depth;
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

float getDepth(String data) {
  Serial.println("Getting depth...");
  Serial.println(data);
  String depth = "";
  char c;
  int measureCount = 1;
  bool hasStarted = false;

  for(int i = 0; i < data.length(); i++) {
    c = data.charAt(i);
    if(measureCount == 9) {
      if(c == '+' || c == '-') {
        if(!hasStarted){
          depth += c;
          hasStarted = true;
        }
        else {
          break;
        }
      }
      else {
        depth += c;
      }
    }
    else {
      if(c == '+' || c == '-'){
        measureCount++;
      }
    }
  }

  return depth.toFloat();
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
    attachInterrupt(5, alarmISR, FALLING);

    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
    detachInterrupt(5);
    
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
    Serial.println("ALARM!");
    if(motorOn) {
      Serial.println("Taking measurement with motor!");
      takeMeasurementWithMotor();
    }
    else {
      takeMeasurement(false);
    }
 
    // set to next alarm
    setAlarmTime();
    
    delay(5000);

}
