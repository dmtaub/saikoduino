/*

*********************************
* Created 23 August 2012          *
* Copyright 2012, Brian Neltner *
* http://saikoled.com           *
* Licensed under GPL3           *
*********************************

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License can be found at
http://www.gnu.org/licenses/gpl.html

Description:
The purpose of this sketch is to combine six established
programs on the saikoduino so that they are automatically
rotated between.

Mode 0: Solid Color Mode
Mode 1: Synchronous filtered saturation and hue random walk.
Mode 2: Rainbow Fade.
Mode 3: Fast Fade.

*/

#define lightid 11 // Light number for use in arrays.

#include "math.h"
#define DEG_TO_RAD(X) (M_PI*(X)/180)

#define redPin 9    // Red LED connected to digital pin 9
#define greenPin 10 // Green LED connected to digital pin 10
#define bluePin 11 // Blue LED connected to digital pin 11
#define whitePin 13 // White LED connected to digital pin 13
#define audioPin 0 // Audio connected to AI0
#define strobePin 4 // MSGEQ7 Strobe Pin
#define resetPin 7 // MSGEQ7 Reset Pin  
#define mode_delaytime 142 // Seconds between mode changes.

// Definitions for solid color fade (i.e. validation mode).
#define solidcolor_delaytime 1 // Seconds between color shifts.
#define solidcolor_idlag 100 // ms between id numbers.

// Definitions for pastel fade (i.e. test mode).
#define pastelfade_steptime 1
#define pastelfade_propgain 0.0005 // "Small Constant"
#define pastelfade_minsaturation 0
#define pastelfade_maxsaturation 0.9
#define pastelfade_idlag 100 // ms between id numbers.

// Definitions for rainbow fade.
#define rainbowfade_steptime 1
#define rainbowfade_idlag 100 // ms between id numbers.

// Definitions for rainbow fade.
#define fastfade_steptime 1

// Definitions for audio responsive mode.
#define audiothresholdvoltage 5 // This voltage above the lowest seen will show up in the scaled value.
#define audiotypicalrange 5 // This is the typical voltage difference between quiet and loud.
#define audioalphashift 0.1 // This is the time constant for shifting the high limit.
#define audioalphareturn 0.001 // This is the time constant for returning to typical values.
#define audioalpharead 0.1 // This is the time constant for filtering incoming audio magnitudes.
#define audioalphalow 0.001 // This is the time constant for moving the low limit.
#define audiodelaytime 1 // Milliseconds between sequential MSGEQ7 readings.

float audiocurrentvalue[7] = {45, 50, 40, 45, 45, 90, 100};
float audioscaledvalue[7];
float audiolimitedvalue[7];

unsigned int audiotypicallow[7] = {45, 50, 40, 45, 45, 90, 100}; // Typical noise floors.
unsigned int audiotypicalhigh[7]; // Approximate max value.

float audiocurrentlow[7] = {45, 50, 40, 45, 45, 90, 100}; // Initial noise floor.
float audiocurrenthigh[7]; // Initial max value.

unsigned int audioband[7] = {63, 160, 400, 1000, 2500, 6250, 16000};

struct HSI {
  float h;
  float s;
  float i;
  float htarget;
  float starget;
} color;

void sendcolor() {
  int rgbw[4];
  while (color.h >=360) color.h = color.h - 360;
  while (color.h < 0) color.h = color.h + 360;
  if (color.i > 1) color.i = 1;
  if (color.i < 0) color.i = 0;
  if (color.s > 1) color.s = 1;
  if (color.s < 0) color.s = 0;
  // Fix ranges (somewhat redundantly).
  hsi2rgbw(color.h, color.s, color.i, rgbw);
  analogWrite(redPin, rgbw[0]);
  analogWrite(greenPin, rgbw[1]);
  analogWrite(bluePin, rgbw[2]);
  analogWrite(whitePin, rgbw[3]);
}

void pastelfade_updatehue() {
  color.htarget += (random(360)-180)*.1;
  color.htarget = fmod(color.htarget, 360);
  color.h += pastelfade_propgain*(color.htarget-color.h);
  color.h = fmod(color.h, 360);
}

void pastelfade_updatesaturation() {
  color.starget += (random(10000)-5000)/0.00001;
  if (color.starget > pastelfade_maxsaturation) color.starget = pastelfade_maxsaturation;
  else if (color.starget < pastelfade_minsaturation) color.starget = pastelfade_minsaturation;
  color.s += pastelfade_propgain*(color.starget-color.s);
  if (color.s > pastelfade_maxsaturation) color.s = pastelfade_maxsaturation;
  else if (color.s < pastelfade_minsaturation) color.s = pastelfade_minsaturation;
}

void rainbowfade_updatehue() {
  color.h += .1;
}

void rainbowfade_updatesaturation() {
  color.s = 1;
}

void fastfade_updatehue() {
  color.h += 5;
}

void fastfade_updatesaturation() {
  color.s = 1;
}

void hsi2rgbw(float H, float S, float I, int* rgbw) {
  int r, g, b, w;
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
    r = S*255*I/3*(1+cos_h/cos_1047_h);
    g = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    b = 0;
    w = 255*(1-S)*I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*255*I/3*(1+cos_h/cos_1047_h);
    b = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
    w = 255*(1-S)*I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*255*I/3*(1+cos_h/cos_1047_h);
    r = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
    w = 255*(1-S)*I;
  }
  
  rgbw[0]=r;
  rgbw[1]=g;
  rgbw[2]=b;
  rgbw[3]=w;
}

// Time-storing Global Variables
long mode_starttime;
long solidcolor_starttime;
long pastelfade_starttime;
long rainbowfade_starttime;
long fastfade_starttime;
long audio_starttime;

enum mode {
  solidcolor,
  pastelfade,
  rainbowfade,
  audio
} mode;

enum solidcolor_state {
  solidcolorinitialize,
  solidcolorpause,
  redchange,
  redrest,
  greenchange,
  greenrest,
  bluechange,
  bluerest,
  whitechange,
  whiterest
} solidcolor_state;

enum pastelfade_state {
  pastelfadeinitialize,
  pastelfadepause,
  activate,
  activatehold,
  changecolor,
  changecolorhold
} pastelfade_state;

enum rainbowfade_state {
  rainbowfadeinitialize,
  rainbowpause,
  rainbowactivate,
  rainbowactivatehold,
  rainbowchangecolor,
  rainbowchangecolorhold
} rainbowfade_state;

enum fastfade_state {
  fastfadeinitialize,
  fastactivate,
  fastactivatehold,
  fastchangecolor,
  fastchangecolorhold
} fastfade_state;

enum audio_state {
  audioinitialize,
  audioanalyze,
  audiohold
} audio_state;

void setup() {
  // Configure LED pins.
  pinMode(whitePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  digitalWrite(whitePin, 0);
  digitalWrite(redPin, 0);
  digitalWrite(greenPin, 0);
  digitalWrite(bluePin, 0);

  pinMode(audioPin, INPUT);
  pinMode(strobePin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, LOW);
  digitalWrite(strobePin, HIGH);
  analogReference(DEFAULT);
  
  // Set prescalers to 1 for all timers; note that this means that millis() et all will be 64x too short.
  TCCR0B = _BV(CS00);
  TCCR1B = _BV(CS10);
  TCCR3B = _BV(CS30);
  TCCR4B = _BV(CS40);
  
  for (int i=0; i<7; i++) {
    audiocurrentlow[i] += audiothresholdvoltage;
    audiotypicallow[i] += audiothresholdvoltage;
    audiocurrenthigh[i] = audiocurrentlow[i] + audiotypicalrange;
    audiotypicalhigh[i] = audiotypicallow[i] + audiotypicalrange;
  }
  
  // Set initial mode.
  mode = solidcolor;
  pastelfade_state = pastelfadeinitialize;
}

void loop() {
  long currenttime = millis()/64;
  float audiointensityred = 0;
  float audiointensitygreen = 0;
  float audiointensityblue = 0;
  float audiofrequencyred = 0;
  float audiofrequencygreen = 0;
  float audiofrequencyblue = 0;
  float audiototalpowerred = 0;
  float audiototalpowergreen = 0;
  float audiototalpowerblue = 0;
  float audiomaxpowerred = 0;
  float audiomaxpowergreen = 0;
  float audiomaxpowerblue = 0;
  switch(mode) {
    case solidcolor:
      switch(solidcolor_state) {
        case solidcolorinitialize:
          mode_starttime = currenttime;
          solidcolor_state = solidcolorpause;
          color.h = 0;
          color.s = 1;
          color.i = 0;
          sendcolor();
          break;
        case solidcolorpause:
          if (currenttime >= lightid*solidcolor_idlag + mode_starttime) solidcolor_state = redchange;
          break;
        case redchange:
          color.h = 0;
          color.s = 1;
          color.i = 1;
          sendcolor();
          solidcolor_starttime = currenttime;
          solidcolor_state = redrest;
          break;
        case redrest:
          if (currenttime >= solidcolor_starttime + 1000 * solidcolor_delaytime) solidcolor_state = greenchange;
          break;
        case greenchange:
          color.h = 120;
          color.s = 1;
          color.i = 1;
          sendcolor();
          solidcolor_starttime = currenttime;
          solidcolor_state = greenrest;
          break;
        case greenrest:
          if (currenttime >= solidcolor_starttime + 1000 * solidcolor_delaytime) solidcolor_state = bluechange;
          break;
        case bluechange:
          color.h = 240;
          color.s = 1;
          color.i = 1;
          sendcolor();
          solidcolor_starttime = currenttime;
          solidcolor_state = bluerest;
          break;
        case bluerest:
          if (currenttime >= solidcolor_starttime + 1000 * solidcolor_delaytime) solidcolor_state = whitechange;
          break;
        case whitechange:
          color.h = 0;
          color.s = 0;
          color.i = 1;
          sendcolor();
          solidcolor_starttime = currenttime;
          solidcolor_state = whiterest;
          break;
        case whiterest:
          if (currenttime >= solidcolor_starttime + 1000 * solidcolor_delaytime) solidcolor_state = redchange;
          break;
      }
      if (currenttime / 1000 >= mode_starttime / 1000 + mode_delaytime) {
        mode = pastelfade;
        pastelfade_state = pastelfadeinitialize;
        color.htarget = color.h;
        color.starget = color.s;
      }
      break;
    case pastelfade:
      switch (pastelfade_state) {
        case pastelfadeinitialize:
          mode_starttime = currenttime;
          pastelfade_state = pastelfadepause;
          randomSeed(0);
          color.h = 0;
          color.s = 1;
          color.i = 0;
          sendcolor();
          break;
        case pastelfadepause:
          if (currenttime >= lightid*pastelfade_idlag + mode_starttime) pastelfade_state = activate;
          break;
        case activate:
          pastelfade_starttime = currenttime;
          sendcolor();
          color.i = color.i + 0.001; // Increase Intensity
          pastelfade_updatehue();
          pastelfade_updatesaturation();
          pastelfade_state = activatehold;
          break;
        case activatehold:
          if (currenttime >= pastelfade_starttime + pastelfade_steptime) pastelfade_state = activate;
          if (color.i >= 1) {
            pastelfade_state = changecolor;
            color.i = 1;
          }
          break;
        case changecolor:
          pastelfade_starttime = currenttime;
          sendcolor();
          pastelfade_updatehue();
          pastelfade_updatesaturation();
          pastelfade_state = changecolorhold;
          break;
        case changecolorhold:
          if (currenttime >= pastelfade_starttime + pastelfade_steptime) pastelfade_state = changecolor;
          break;
      }
      if (currenttime / 1000 >= mode_starttime / 1000 + mode_delaytime) {
        mode = audio;
        audio_state = audioinitialize;
      }
      break;
    case rainbowfade:
      switch (rainbowfade_state) {
        case rainbowfadeinitialize:
          mode_starttime = currenttime;
          rainbowfade_state = rainbowpause;
          color.h = 0;
          color.s = 1;
          color.i = 0;
          sendcolor();
          break;
        case rainbowpause:
          if (currenttime >= lightid*rainbowfade_idlag + mode_starttime) rainbowfade_state = rainbowactivate;
          break;
        case rainbowactivate:
          rainbowfade_starttime = currenttime;
          sendcolor();
          color.i = color.i + 0.001; // Increase Intensity
          rainbowfade_updatehue();
          rainbowfade_updatesaturation();
          rainbowfade_state = rainbowactivatehold;
          break;
        case rainbowactivatehold:
          if (currenttime >= rainbowfade_starttime + rainbowfade_steptime) rainbowfade_state = rainbowactivate;
          if (color.i >= 1) {
            rainbowfade_state = rainbowchangecolor;
            color.i = 1;
          }
          break;
        case rainbowchangecolor:
          rainbowfade_starttime = currenttime;
          sendcolor();
          rainbowfade_updatehue();
          rainbowfade_updatesaturation();
          rainbowfade_state = rainbowchangecolorhold;
          break;
        case rainbowchangecolorhold:
          if (currenttime >= rainbowfade_starttime + rainbowfade_steptime) rainbowfade_state = rainbowchangecolor;
          break;
      }
      if (currenttime / 1000 >= mode_starttime / 1000 + mode_delaytime) {
        mode = solidcolor;
        solidcolor_state = solidcolorinitialize;
      }
      break;
    case audio:
      switch (audio_state) {
        case audioinitialize:
          mode_starttime = currenttime; 
          audio_state = audioanalyze;
          digitalWrite(redPin, 0);
          digitalWrite(greenPin, 0);
          digitalWrite(bluePin, 0);
          digitalWrite(whitePin, 0);
          break;
        case audioanalyze:
          digitalWrite(resetPin, HIGH);
          digitalWrite(resetPin, LOW);
          delayMicroseconds(72);

          for(int i=0; i<7; i++) {
            digitalWrite(strobePin, LOW);
            delayMicroseconds(35);
            unsigned int audiomomentaryvalue = analogRead(audioPin); // Get ADC Value.
    
            // Filter input audio signal.
            audiocurrentvalue[i] = audioalpharead*(float)audiomomentaryvalue + (1-audioalpharead)*(float)audiocurrentvalue[i];
            
            // Guarantee that the currentlow is not going to cause negative values in comparisons.
            if (audiocurrentlow[i]<audiothresholdvoltage) audiocurrentlow[i]=audiothresholdvoltage;
            
            // Set default limited value.
            audiolimitedvalue[i] = audiocurrentvalue[i];

            // Automatically generate an expected "current low" value as being the filtered current value plus a threshold.
            audiocurrentlow[i] = audioalphalow*((float)audiocurrentvalue[i]+audiothresholdvoltage) + (1-audioalphalow)*(float)audiocurrentlow[i];
    
            // If the value is higher than anything seen yet, it is the new high and limit the value.
            if (audiocurrentvalue[i] > audiocurrenthigh[i]) {
              audiocurrenthigh[i] = audioalphashift*(float)audiocurrentvalue[i] + (1-audioalphashift)*(float)audiocurrenthigh[i];
              audiolimitedvalue[i] = audiocurrenthigh[i];
            }
    
            // Limit the value based on the current low.
            if (audiocurrentvalue[i] < audiocurrentlow[i]) audiolimitedvalue[i] = audiocurrentlow[i];
            
            // Map [currentlow, currenthigh] to [0, 0xFF].
  
            audioscaledvalue[i] = (float)(audiolimitedvalue[i]-audiocurrentlow[i])/(float)(audiocurrenthigh[i]-audiocurrentlow[i])*0xFF;
        
            if (i<2) audiointensityred += audioscaledvalue[i]/2;
            else if (i<4) audiointensitygreen += audioscaledvalue[i]/2;
            else if (i<5) audiointensityblue += audioscaledvalue[i];
      
            audiocurrenthigh[i] = (1-audioalphareturn)*audiocurrenthigh[i]+audioalphareturn*(audiocurrentlow[i]+audiotypicalrange);
    
            digitalWrite(strobePin, HIGH);
            delayMicroseconds(72);
          }

          analogWrite(redPin, audiointensityred);
          analogWrite(greenPin, audiointensitygreen);
          analogWrite(bluePin, audiointensityblue);

          audio_state = audiohold;
          break;
        case audiohold:
          if (currenttime >= audio_starttime + audiodelaytime) audio_state = audioanalyze;
          break;
      }
      if (currenttime / 1000 >= mode_starttime / 1000 + mode_delaytime) {
        mode = rainbowfade;
        rainbowfade_state = rainbowfadeinitialize;
      }
      break;
  }  
}
