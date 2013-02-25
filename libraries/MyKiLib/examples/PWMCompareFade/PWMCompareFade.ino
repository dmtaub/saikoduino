/*
* This sketch demonstrates the difference between 8 and 16 bit PWM fades
*
* Copyright 2013,  Daniel Taub                    
* http://saikoled.com                             
*/
  
#include <MyKi.h>

MyKi light;
int i;

void setup() {    
  // constructor sets up 16 bit PWM
  light=MyKi();
  i=0;
}

void loop() {
  // this function replaces analogWrite for the 4 LED pins, 8 bit PWM simulation
  //light.rgbw8Send(0,i,0,0);
  
  // this function allows the full range of integer values to be used for PWM
  light.rgbwSend(i,i>>8<<8,0,0);

  i++;  
  delay(10);

}
