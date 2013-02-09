#define redPin 9    
#define greenPin 10 
#define bluePin 11 // Blue LED connected to digital pin 11
#define whitePin 13 // White LED connected to digital pin 13

#define audioPin 0 // Audio connected to AI0
#define strobePin 4 // MSGEQ7 Strobe Pin
#define resetPin 7 // MSGEQ7 Reset Pin  
#define mode_delaytime 142 // Seconds between mode changes.

#define steptime 1


#define audiothresholdvoltage 5 // This voltage above the lowest seen will show up in the scaled value.
#define audiotypicalrange 5 // This is the typical voltage difference between quiet and loud.
#define audioalphashift 0.1 // This is the time constant for shifting the high limit.
#define audioalphareturn 0.001 // This is the time constant for returning to typical values.
#define audioalpharead 0.1 // This is the time constant for filtering incoming audio magnitudes.
#define audioalphalow 0.001 // This is the time constant for moving the low limit.
#define audiodelaytime 1 // Milliseconds between sequential MSGEQ7 readings.

enum audio_state {
  audioinitialize,
  audioanalyze,
  audiohold
} audio_state;

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

long audio_starttime;



#define SIZE 3 // max frame size, if you want to do it this way
volatile byte dmx_buffer[SIZE]; // allocate buffer space

volatile unsigned int dmx_ptr = 0;
volatile byte dmx_state = 0; // state tracker

void setup() {
  pinMode(redPin, OUTPUT);  
  pinMode(greenPin, OUTPUT);  
  pinMode(bluePin, OUTPUT);  
  pinMode(whitePin, OUTPUT);  

  // Enable output on port to control direction of RS485
  DDRD |= 0x03;
  PORTD |= 0x01;
  
  // initialize uart for data transfer
  UBRR1H = ((F_CPU/250000/16) - 1) >> 8;
  UBRR1L = ((F_CPU/250000/16) - 1);
  UCSR1A = 1<<UDRE1;
  UCSR1C = 1<<USBS1 | 1<<UCSZ11 | 1<<UCSZ10; // 2 stop bits,8 data bitss
  UCSR1B = 1<<TXEN1; // turn it on

  sei();
  analogWrite(whitePin,10);

  // audio
  audio_state = audioinitialize;

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
  
  switch (audio_state) {
        case audioinitialize:
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
  
  while(dmx_state); // check if last buffer was sent out
  // fill buffer with some new data
  for (byte i = 0; i < SIZE/3; i++) {
    dmx_buffer[i] = audiointensityred;
    dmx_buffer[i+1] = audiointensitygreen;
    dmx_buffer[i+2] = audiointensityblue;
  }
  // send out dmx data
  dmx_send();

}


ISR(USART1_TX_vect) {

  // handle data transfers
  // check for state 3 first as its most probable
  if (dmx_state == 3) {
    UDR1 = dmx_buffer[dmx_ptr];
    dmx_ptr++;
    if (dmx_ptr >= SIZE) {
      dmx_state = 4; // last byte sent
    }
  }
  else if (dmx_state == 1) {
    // reset baudrate generator
    UBRR1H = ((F_CPU/250000/16) - 1) >> 8;
    UBRR1L = ((F_CPU/250000/16) - 1);
    UDR1 = 0; // send slot0 blank space
    dmx_state = 2; // set to slot0 transmit mode
  }
  else if (dmx_state == 2) {
    UDR1 = dmx_buffer[0]; // send first data byte
    dmx_ptr = 1; // set pointer to next data byte
    dmx_state = 3; // set to data transfer mode
  }
  else {
    UCSR1B &= ~(1<<TXCIE1); // disable tx complete interrupt
    UCSR1A |= 1<<TXC1; // clear flag if set
    dmx_state = 0; // reset - bad condition or done

  }
}

void dmx_send() {
  dmx_state = 1; // set to BREAK transfer
  // set usart for really slow transfer for BREAK signal
  // this will produce a BREAK of 112.5us
  // and a MAB of 25us
  UBRR1H = ((F_CPU/80000/16) - 1) >> 8;
  UBRR1L = ((F_CPU/80000/16) - 1);
  UCSR1B |= 1<<TXCIE1; // enable tx complete interrupt
  UDR1 = 0;
  
}

/*ISR(USART1_RX_vect) {

  // handle data transfers
  byte temp = UDR1; // get data
  // check if BREAK byte
  if ((UCSR1A & 0x10) == 0x10){//1<<FE1) {
    //dmx_state = 1; // set to slot0 byte waiting
    //PORTB |= 0x04;
    if (temp) {
      dmx_state = 0; // error - reset reciever
    }
    else {
      dmx_ptr = 0; // reset buffer pointer to beginning
      dmx_state = 2; // set to data waiting
    }
  }
  // check for state 2 first as its most probable
  else if (dmx_state == 2) {
    dmx_buffer[dmx_ptr] = temp;
    dmx_ptr++;
    if (dmx_ptr >= SIZE) {
      dmx_state = 0; // last byte recieved or error
    }
  }
  else if (dmx_state == 1) {
    // check if slot0 = 0
    if (temp) {
      dmx_state = 0; // error - reset reciever
    }
    else {
      dmx_ptr = 0; // reset buffer pointer to beginning
      dmx_state = 2; // set to data waiting
    }
  }
  else {
    dmx_state = 0; // reset - bad condition or done
  }
     PORTB &= 0xFB;

}
*/
