#define redPin 9    // Red LED connected to digital pin 9
#define greenPin 10 // Green LED connected to digital pin 10
#define bluePin 11 // Blue LED connected to digital pin 11
#define whitePin 13 // White LED connected to digital pin 13

#include <MyKi.h>
MyKi light;

// the setup routine runs once when you press reset:
void setup() {    

}

// the loop routine runs over and over again forever:
void loop() {
  light.rgbw8Send(1,1,1,0);
  delay(500);
  //light.rgbwSend(0,0,0,0);
  delay(500);
  //light.rgbwSend(1,1,1,0);
  delay(1000);            // wait for a second

  
}
