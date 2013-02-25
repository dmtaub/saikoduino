/*
* This sketch demonstrates an HSI fade using MyKi Library with 16-bit PWM 
*
* Copyright 2013,  Daniel Taub                    
* http://saikoled.com                             
*/
  
#include <MyKi.h>

MyKi light;
float hue;

#define INC 0.1
#define DELAY 2  //in milliseconds

void setup() {    
  light=MyKi();
  hue = 0;
}

void loop() {
  light.hsiSend(hue+INC,1,1);
  hue=hue+INC;
  while (hue >= 360.0)
    hue -= 360;
  delay(DELAY);
}
