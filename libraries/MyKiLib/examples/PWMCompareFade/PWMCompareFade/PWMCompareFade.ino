/*
* This sketch demonstrates the difference between 8 and 16 bit PWM fades
* More noticeable at low light levels.
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
  // 0xFF = 255
  //light.rgbw8Send(0,0xFF,0,0);
  
  // this function allows the full range of integer values to be used for PWM
  // 0xFFFF = 65535
  // light.rgbwSend(0,0xFFFF,0,0);
  
  // bit shifting to simulate what 8-bit color looks like. 
  light.rgbwSend(abs(i*2),abs(i*2)>>8<<8,0,0);
  // abs(i*2) because i as signed integer goes from 0 to 32767 
  // then overflows to -32767 for the fade down

  i++; 

  delay(10);

}
