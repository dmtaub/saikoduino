/*
  MyKi.h - Library for flashing MyKi lights
  Created by Daniel M. Taub, Feb 24, 2013
  Released under GPLv3
*/

#ifndef MyKi_h
#define MyKi_h

#include "WProgram.h"

class MyKi
{
  public:
    MyKi(int pin);
    void dot();
    void dash();
  private:
    int _pin;
};

#endif

