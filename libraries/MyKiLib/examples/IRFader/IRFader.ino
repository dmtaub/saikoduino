/*
* This sketch demonstrates 16bit PWM fades controllable by IR-activated interrupts
*
* Copyright 2013,  Daniel Taub                    
* http://saikoled.com                             
*/
  
#include <MyKi.h>
#include <IRremote.h>

#define RECV_PIN 16

IRrecv irrecv(RECV_PIN);
decode_results results;

MyKi light;
int scaler = 0;

void checkIr();

void setup() {    
  // constructor sets up 16 bit PWM
  light=MyKi();
  irrecv.enableIRIn(); // Start the receiver
  Serial.begin(9600);
}

void loop() {
  checkIr();
  // this function replaces analogWrite for the 4 LED pins, 8 bit PWM simulation
  light.rgbwSend(1,0,0,0);
  delay(5);
  /// this function allows the full range of integer values to be used for PWM
  //light.rgbwSend(1,0,0,0);
  //delay(50);             
}

void checkIr(){
 if (irrecv.decode(&results)) {   
   if (results.value & 0xF000) {}
     else { 
      switch(results.value & 0x0FF) {   
       //      remote topbottom leftright: C,20,21,D,11,10,38
        case 0xC:
          //mode=lightsoff; 
          break;
        case 0x20:
          break;
        case 0x21:
          break;
        case 0xD:
          //mode = audio;
          break;
        case 0x11:
          scaler -= 1;
          light.setScale(scaler);
          //mode = pastelfade;
          break;
        case 0x10:
          scaler += 1;
          light.setScale(scaler);
          //mode = rainbowfade;
          break;
        case 0x38:
          //mode = solidcolor;
          break;
      }
    }
    irrecv.resume(); // Receive the next value
  }
}
