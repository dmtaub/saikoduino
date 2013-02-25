/*
  MyKi.h - Library for flashing MyKi lights
  Created by Daniel M. Taub, Feb 24, 2013
  Released under GPLv3

  This library assumes the following pin definitions:

  #define redPin 9    // Red LED connected to digital pin 9
  #define greenPin 10 // Green LED connected to digital pin 10
  #define bluePin 11 // Blue LED connected to digital pin 11
  #define whitePin 13 // White LED connected to digital pin 13
^ will change for production release

#include "WProgram.h"
#include "MyKi.h"
#include "math.h"

#define DEG_TO_RAD(X) (M_PI*(X)/180)

#define LINEAR true

#define RED OCR1A
#define GREEN OCR1B
#define BLUE OCR1C
#define WHITE OCR4A

MyKi::MyKi()
{
  // This section is derived from Brian Neltner's work detailed at
  // http://blog.saikoled.com/post/43165849837/secret-konami-cheat-code-to-high-resolution-pwm-on-your
  
  // Pin Setup
  // Change the data direction on B5, B6, and B7 to 
  // output without overwriting the directions of B0-4.
  // B5/OC1A is RED, B6/OC1B is GREEN, and B7/OC1C is BLUE.

  DDRB |= (1<<7)|(1<<6)|(1<<5);

  // PWM Setup Stuff
  // TCCR1A is the Timer/Counter 1 Control Register A.
  // TCCR1A[7:6] = COM1A[1:0] (Compare Output Mode for Channel A)
  // TCCR1A[5:4] = COM1B[1:0] (Compare Output Mode for Channel B)
  // TCCR1A[3:2] = COM1C[1:0] (Compare Output Mode for Channel C)
  // COM1x[1:0] = 0b10 (Clear OC1x on compare match. Set at TOP)
  // TCCR1A[1:0] = WGM1[1:0] (Waveform Generation Mode for Timer 1 LSB)
  // TCCR1B[4:3] = WGM1[3:2] (Waveform Generation Mode for Timer 1 MSB)
  // WGM[3:0] = 0b1110 (Mode 14)
  // Mode 14 - Fast PWM, TOP=ICR1, Update OCR1x at TOP, TOVn Flag on TOP
  // So, ICR1 is the source for TOP.

  // Clock Stuff
  // TCCR1B[2:0] = CS1[2:0] (Clock Select for Timer 1)
  // CS1[2:0] = 001 (No Prescaler)

  // Unimportant Stuff
  // TCCR1B7 = ICNC1 (Input Capture Noise Canceler - Disable with 0)
  // TCCR1B6 = ICES1 (Input Capture Edge Select - Disable with 0)
  // TCCR1B5 = Reserved

  // And put it all together and... tada!
  TCCR1A = 0b10101010; 
  TCCR1B = 0b00011001;

  // Writes 0xFFFF to the ICR1 register. THIS IS THE SOURCE FOR TOP.
  // This defines our maximum count before resetting the output pins
  // and resetting the timer. 0xFFFF means 16-bit resolution.

  ICR1 = 0xFFFF;

  // Example of setting 16-bit PWM value.

  // Initial RGB values.
  RED = 0x0000;
  GREEN = 0x0000;
  BLUE = 0x0000;
  
  // Similar setup for White. Mostly a temporary hack for the rev7
  // since the rev10 will have white connected to timer 3's 16-bit
  // PWM output. Will change for production release.
  
  DDRC |= (1<<7); // White LED PWM Port.
  
  // Should enable OCR4A, enable the PWM on A, and set prescale
  // to 256.
  
  // Magic sauce to do 10-bit register writing.
  // Write to TC4H first also when writing to white OCR4A.
  TC4H = 0x03;
  OCR4C = 0xFF;
  
  // Configuration of Timer 4 Registers.
  TCCR4A = 0b10000010;
  TCCR4B = 0b00000001;
  
  // Jeez that's a lot of registers. What a fancy timer.  pinMode(pin, OUTPUT);
}

void MyKi::rgbwSend(int r, int g, int b, int w)
{
  _r = r;
  _g = g;
  _b = b;
  _w = w;
  send();
}

void MyKi::hsiSend(float h, float s, float v)
{
  hsi2rgb(h,s,v);
  send();
}
void MyKi::send()
{
  scale();
  RED = _r;
  GREEN = _g;
  BLUE = _b;
  TC4H = _w >> 8; // High 2 bits of white PWM.
  WHITE = 0xFF & _w; // Low 8 bits of white PWM.
}
void MyKi::scale(){
  if (_scale == 0)
   return;
  else if (_scale < 0)
   {
     _r >> -_scale;
     _g >> -_scale;
     _b >> -_scale;
     _w >> -_scale;
   }
  else 
   {
     _r << _scale;
     _g << _scale;
     _b << _scale;
     _w << _scale;
   }

}

void hsi2rgbw(float H, float S, float I) {
  float r, g, b, w;
  float cos_h, cos_1047_h;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
  
  // This section is modified by the addition of white so that it assumes 
  // fully saturated colors, and then scales with white to lower saturation.
  //
  // Next, scale appropriately the pure color by mixing with the white channel.
  // Saturation is defined as "the ratio of colorfulness to brightness" so we will
  // do this by a simple ratio wherein the color values are scaled down by (1-S)
  // while the white LED is placed at S.
  
  // This will maintain constant brightness because in HSI, R+B+G = I. Thus, 
  // S*(R+B+G) = S*I. If we add to this (1-S)*I, where I is the total intensity,
  // the sum intensity stays constant while the ratio of colorfulness to brightness
  // goes down by S linearly relative to total Intensity, which is constant.

  if(H < 2.09439) {
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    r = S*I/3*(1+cos_h/cos_1047_h);
    g = S*I/3*(1+(1-cos_h/cos_1047_h));
    b = 0;
    w = (1-S)*I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*I/3*(1+cos_h/cos_1047_h);
    b = S*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
    w = (1-S)*I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*I/3*(1+cos_h/cos_1047_h);
    r = S*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
    w = (1-S)*I;
  }
  
  // Mapping Function from rgbw = [0:1] onto their respective ranges.
  // For standard use, this would be [0:1]->[0:0xFFFF] for instance.
  if (LINEAR)
  { 
    _r = (0xFFFF*r);
    _g = (0xFFFF*g);
    _r = (0xFFFF*b);
    _w = (0x3FFF*w);
  }
  else {
    // Here instead I am going to try a parabolic map followed by scaling.  
    _r=0xFFFF*r*r;
    _g=0xFFFF*g*g;
    _b=0xFFFF*b*b;
    _w=0x3FF*w*w;
  }
}

