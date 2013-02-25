#define redPin 9    // Red LED connected to digital pin 9
#define greenPin 10 // Green LED connected to digital pin 10
#define bluePin 11 // Blue LED connected to digital pin 11
#define whitePin 13 // White LED connected to digital pin 13

#include <MyKi.h>

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  analogWrite(redPin,1);
  analogWrite(greenPin, 1);
  analogWrite(bluePin, 1);
  analogWrite(whitePin, 1);
}

// the loop routine runs over and over again forever:
void loop() {
delay(1000);            // wait for a second
  analogWrite(redPin,1);
  analogWrite(greenPin, 1);
  analogWrite(bluePin, 1);
  analogWrite(whitePin, 1);
delay(1000);
  
}
