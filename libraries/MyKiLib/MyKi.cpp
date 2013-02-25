/*
  MyKi.h - Library for flashing MyKi lights
  Created by Daniel M. Taub, Feb 24, 2013
  Released under GPLv3
*/

#include "WProgram.h"
#include "MyKi.h"

MyKi::MyKi(int pin)
{
  pinMode(pin, OUTPUT);
  _pin = pin;
}

void MyKi::dot()
{
  digitalWrite(_pin, HIGH);
  delay(250);
  digitalWrite(_pin, LOW);
  delay(250);  
}

void MyKi::dash()
{
  digitalWrite(_pin, HIGH);
  delay(1000);
  digitalWrite(_pin, LOW);
  delay(250);
}

