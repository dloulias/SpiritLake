#include <SDI12.h>
#include <SPI.h>
#include <SD.h>

#define DATAPIN 10
#define CHIPSELECT 53

SDI12 mySDI12(DATAPIN);

bool endProgram;

void setup() {
  Serial.begin(9600);
  mySDI12.begin();
  endProgram = false;

}

void loop() {
  while(!endProgram) {
    takeMeasurement();
    endProgram = true;
  }

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

