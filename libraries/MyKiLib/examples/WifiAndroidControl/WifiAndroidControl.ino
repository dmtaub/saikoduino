/*
* This sketch demonstrates an HSI fade using MyKi Library with 16-bit PWM 
*
* Copyright 2013,  Daniel Taub                    
* http://saikoled.com                             
*/
  
#include <MyKi.h>

MyKi light;
float hue;
bool isSerial=false;
char buf[80];
int rgbw[4];
unsigned long time;

int index=0;

#define INC 0.1
#define DELAY 2  //in milliseconds

void setup() {    
  light=MyKi();
  hue = 0;
  Serial.begin(57600);
  Serial1.begin(9600);
}

void parseMessage(){
  char *p = buf;
  char *str;
  int item=0;
  while ((str = strtok_r(p, ",", &p)) != NULL){ // delimiter is the semicolon
    rgbw[item]=atoi(str);
    item++;
   }
   if (item == 4){
     isSerial = true;
     time = millis();
     light.rgbw8Send(rgbw[0],rgbw[1],rgbw[2],rgbw[3]>>2);
   }
}

void checkSerial(){ 
  Serial.flush();
  char inch = '\0';
  while (Serial1.available()){
    inch = (Serial1.read());
    //Serial.write(inch);
    if (inch == '\n'){
      index = 0;
      buf[index]=NULL;
    }
    else if (inch == '\r'){
      parseMessage();
    }
    else{
      buf[index]=inch;
      index++;
      buf[index]='\0';
    }
  }
}

void loop() {
  checkSerial();
  if (isSerial && (millis() - time <= 20000)){}
  else{
    light.hsiSend(hue+INC,1,1);
    hue=hue+INC;
    while (hue >= 360.0)
      hue -= 360;
    delay(DELAY);
  }
}
