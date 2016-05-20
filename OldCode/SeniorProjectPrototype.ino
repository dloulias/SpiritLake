#include <Wire.h>
#include "rgb_lcd.h"
#include "RTClib.h"
#include <SDI12.h> 

int programPin = 2;
int potPin = A1;
int downPin = 3;
int upPin = 4;
int dataPin = 10;

RTC_DS1307 my_rtc;
rgb_lcd lcd;
SDI12 mySDI12(dataPin);

int readingHour = 22;
int readingMinute = 59;

void setup() {
  pinMode(programPin, INPUT);
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);
  
  //attachInterrupt(digitalPinToInterrupt(programPin), button_press, CHANGE);
  Serial.begin(57600);
  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 255);
}

void loop() {

  
  if(digitalRead(programPin) == 1) 
  {
    lcd.clear();
    set_time();
  }

  if(digitalRead(downPin) == 1)
  {
    lcd.clear();
    set_date();
    display_time();
    delay(1000);
  }

  if(digitalRead(upPin) == 1)
  {
    lcd.clear();
    set_reading_time();
    delay(1000);
  }

  DateTime now = my_rtc.now();
  if(now.hour() == readingHour && now.minute() == readingMinute)
  {
    take_reading();
  }

  lcd.clear();
  display_time();
  delay(100);
  
  // put your main code here, to run repeatedly:

}

void take_reading()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Taking Reading");

  mySDI12.sendCommand("0!"); 
  delay(5000); 

  
}
void set_reading_time()
{
  
  lcd.setCursor(0,0);
  lcd.print("Set hour:");
  lcd.setRGB(0, 255, 0);
  delay(500);
  readingHour = getHour();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set minute:");
  delay(700);
  readingMinute = getMinute();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reading Time Set:");
  lcd.setCursor(0,1);
  lcd.print(readingHour);
  lcd.print(':');
  if(readingMinute < 10)
    lcd.print(0);
  lcd.print(readingMinute);

}


void display_time() {
  DateTime now = my_rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  
  lcd.setRGB(0, 204, 204);
  lcd.setCursor(0, 0);
  lcd.print("Date/Time:");
  lcd.setCursor(0,1);
  lcd.print(month);
  lcd.print('/');
  lcd.print(day);
  lcd.print('/');
  lcd.print(year);
  lcd.print(' ');
  lcd.print(hour);
  lcd.print(':');
  if(minute < 10)
    lcd.print('0');
  lcd.print(minute);
  Serial.print(minute);
}

void set_date()
{
  lcd.setCursor(0,0);
  lcd.print("Set year:");
  lcd.setRGB(0, 255, 0);
  delay(500);
  int year = getYear();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set month:");
  delay(700);
  int month = getMonth();
  

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set day:");
  delay(700);
  int day = getDay(month);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Day Set:");
  lcd.setCursor(0,1);
  if(month < 10)
    lcd.print(0);
  lcd.print(month);
  lcd.print('/');
  if(day < 10)
    lcd.print(0);
  lcd.print(day);
  lcd.print('/');
  lcd.print(year);

  DateTime now = my_rtc.now();

  my_rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), 0));
  
}

int getYear()
{
  int year = 2016;
  lcd.setCursor(0,1);
  lcd.print(year);
  Serial.print("Entering getyear");
  //noInterrupts();
  while(1)
  {
    if(digitalRead(upPin) == HIGH)
    {
      if(year < 2099)
      {
        year++;
        lcd.setCursor(0,1);
        lcd.print(year);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go higher");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set year:");
        lcd.setCursor(0,1);
        lcd.print(year);
      }

      delay(500);
    }
    if(digitalRead(downPin) == HIGH)
    {
      if(year > 2016)
      {
        year--;
        lcd.setCursor(0,1);
        lcd.print(year);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go lower");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set year:");
        lcd.setCursor(0,1);
        lcd.print(year);
      }
      delay(500);
    }
    if(digitalRead(programPin) == HIGH)
    {
      delay(700);
      break;
    }
  }

  return year;
}

int getMonth()
{
  int month = 1;
  lcd.setCursor(0,1);
  lcd.print(month);
  //noInterrupts();
  while(1)
  {
    if(digitalRead(upPin) == HIGH)
    {
      if(month < 12)
      {
        month++;
        lcd.setCursor(0,1);
        lcd.print(month);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go higher");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set month:");
        lcd.setCursor(0,1);
        lcd.print(month);
      }

      delay(500);
    }
    if(digitalRead(downPin) == HIGH)
    {
      if(month > 1)
      {
        month--;
        lcd.setCursor(0,1);
        lcd.print(month);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go lower");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set month:");
        lcd.setCursor(0,1);
        lcd.print(month);
      }
      delay(500);
    }
    if(digitalRead(programPin) == HIGH)
    {
      delay(700);
      break;
    }
  }

  return month;
}

int getDay(int month)
{
  int day = 1;
  lcd.setCursor(0,1);
  lcd.print(day);
  //noInterrupts();
  while(1)
  {
    if(digitalRead(upPin) == HIGH)
    {
      if(day < 31)
      {
        day++;
        lcd.setCursor(0,1);
        lcd.print(day);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go higher");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set day:");
        lcd.setCursor(0,1);
        lcd.print(day);
      }

      delay(500);
    }
    if(digitalRead(downPin) == HIGH)
    {
      if(day > 1)
      {
        day--;
        lcd.setCursor(0,1);
        lcd.print(day);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go lower");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set day:");
        lcd.setCursor(0,1);
        lcd.print(day);
      }
      delay(500);
    }
    if(digitalRead(programPin) == HIGH)
    {
      delay(700);
      break;
    }
  }

  return day;
}

void set_time()
{
  
  lcd.setCursor(0,0);
  lcd.print("Set hour:");
  lcd.setRGB(0, 255, 0);
  delay(500);
  int hour = getHour();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set minute:");
  delay(700);
  int minute = getMinute();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Time Set:");
  lcd.setCursor(0,1);
  lcd.print(hour);
  lcd.print(':');
  if(minute < 10)
    lcd.print(0);
  lcd.print(minute);

  DateTime now = my_rtc.now();

  my_rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, 0));
}

int getHour()
{
  int hour = 0;
  lcd.setCursor(0,1);
  lcd.print(hour);
  //noInterrupts();
  while(1)
  {
    if(digitalRead(upPin) == HIGH)
    {
      if(hour < 23)
      {
        hour++;
        lcd.setCursor(0,1);
        lcd.print(hour);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go higher");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set hour:");
        lcd.setCursor(0,1);
        lcd.print(hour);
      }

      delay(500);
    }
    if(digitalRead(downPin) == HIGH)
    {
      if(hour > 0)
      {
        hour--;
        lcd.setCursor(0,1);
        lcd.print(hour);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go lower");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set hour:");
        lcd.setCursor(0,1);
        lcd.print(hour);
      }
      delay(500);
    }
    if(digitalRead(programPin) == HIGH)
    {
      delay(700);
      break;
    }
  }

  return hour;
}

int getMinute()
{
  int minute = 0;
  lcd.setCursor(0,1);
  lcd.print(minute);
  //noInterrupts();
  while(1)
  {
    if(digitalRead(upPin) == HIGH)
    {
      if(minute < 59)
      {
        minute++;
        lcd.setCursor(0,1);
        lcd.print(minute);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go higher");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set minute:");
        lcd.setCursor(0,1);
        lcd.print(minute);
      }

      delay(500);
    }
    if(digitalRead(downPin) == HIGH)
    {
      if(minute > 0)
      {
        minute--;
        lcd.setCursor(0,1);
        lcd.print(minute);
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Cannot go lower");
        delay(700);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set minute:");
        lcd.setCursor(0,1);
        lcd.print(minute);
      }
      delay(500);
    }
    if(digitalRead(programPin) == HIGH)
    {
      delay(700);
      break;
    }
  }

  return minute;
}




