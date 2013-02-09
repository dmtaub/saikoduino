/* Feb 9 2013

dmt@saikoled.com
guest@openmusiclabs.com

*/


#define redPin 9    // Red LED connected to digital pin 9
#define greenPin 10// Green LED connected to digital pin 10 
#define bluePin 11 // Blue LED connected to digital pin 11
#define whitePin 13 // White LED connected to digital pin 13

#define steptime 1

// the loop routine runs over and over again forever:
/*void loop() {
  delay(1000);               // wait for a second
  analogWrite(whitePin, 10);  

}
*/
#define SIZE 512 // max frame size, if you want to do it this way
volatile byte dmx_buffer[SIZE]; // allocate buffer space

volatile unsigned int dmx_ptr = 0;
volatile byte dmx_state = 0; // state tracker

void setup() {
  pinMode(redPin, OUTPUT);  
  pinMode(greenPin, OUTPUT);  
  pinMode(bluePin, OUTPUT);  
  pinMode(whitePin, OUTPUT);  
  DDRD |= 0x03;
  //DDRB |= 0x04;
  PORTD &= 0xfc;
  
  // initialize uart for data transfer
  UBRR1H = ((F_CPU/250000/16) - 1) >> 8;
  UBRR1L = ((F_CPU/250000/16) - 1);
  UCSR1A = 1<<UDRE1;
  UCSR1C = 1<<USBS1 | 1<<UCSZ11 | 1<<UCSZ10; // 2 stop bits,8 data bitss
  UCSR1B = 1<<RXCIE1 | 1<<RXEN1; // turn it on, set RX complete intrrupt on
  sei();
  analogWrite(whitePin,10);
}

void loop() {
  // use the data in dmx_buffer[] here
  analogWrite(redPin,dmx_buffer[0]);
  analogWrite(greenPin,dmx_buffer[1]);
  analogWrite(bluePin,dmx_buffer[2]);
  //analogWrite(whitePin,dmx_buffer[3]);
  analogWrite(whitePin,0);

}

ISR(USART1_RX_vect) {

  // handle data transfers
  // check if BREAK byte
  //if ((UCSR1A & 0x10) == 0x10){
    if (UCSR1A &(1<<FE1)) {
      byte temp = UDR1; // get data 
      dmx_state = 1; // set to slot0 byte waiting
      //PORTB |= 0x04;
    
  }
  // check for state 2 first as its most probable
  else if (dmx_state == 2) {
    dmx_buffer[dmx_ptr] = UDR1;
    dmx_ptr++;
    if (dmx_ptr >= SIZE) {
      dmx_state = 0; // last byte recieved or error
    }
  }
  else if (dmx_state == 1) {
    // check if slot0 = 0
    if (UDR1) {
      dmx_state = 0; // error - reset reciever
    }
    else {
      dmx_ptr = 0; // reset buffer pointer to beginning
      dmx_state = 2; // set to data waiting
    }
  }
  else {
    byte temp = UDR1;
    dmx_state = 0; // reset - bad condition or done
  }
  //PORTB &= 0xFB;

}

