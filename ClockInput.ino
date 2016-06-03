/*
 * This program takes user input to set a 3231 clock.
 * The program button must be pressed to begin programming.
 * Demetra Loulias
 * 
 * 1602 Pins:
 * Vss = GND
 * Vdd = +5V
 * V0 = GND (Could change when we use backlight)
 * RS = 8
 * RW = GND
 * E = 9
 * D4 = 4
 * D5 = 5
 * D6 = 6
 * D7 = 7
 * 
 * Button Pins:
 * Program = 22
 * Up = 26
 * Down = 24
 * 
 * Transistor Gate Pin = 14
 */

#define TIMEUP 26
#define TIMEDOWN 24
#define PROGRAM 22
#define LCDON 14

#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>
#include "LiquidCrystal.h"
LiquidCrystal lcd(8,9,4,5,6,7);

void setup() {
  Serial.begin(115200);
  
  pinMode(TIMEUP, INPUT_PULLUP);
  digitalWrite(TIMEUP, HIGH);
  pinMode(TIMEDOWN, INPUT_PULLUP);
  digitalWrite(TIMEDOWN, HIGH);
  pinMode(PROGRAM, INPUT_PULLUP);
  digitalWrite(PROGRAM, HIGH);

  pinMode(LCDON, OUTPUT);
  
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
  
  delay(2000);


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

  lcd.clear();
  digitalWrite(LCDON, LOW);
  
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

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(PROGRAM) == LOW) {
    programClock();
    delay(500);
  }

}
