/*
 * Saikoduino Web Server
 * Daniel Taub 2012
 * SaikoLED 2012
 *
 * Based on httpserver example from WiFlyHQ library
 *
 * Displays a web form for input to change the settings on the Saikoduino light.
 *
 * 
 */

 /* Notes:
  * Uses chunked message bodies to work around a problem where
  * the WiFly will not handle the close() of a client initiated
  * TCP connection. It fails to send the FIN to the client.
  * (WiFly RN-XV Firmware version 2.32).
  */

/* Work around a bug with PROGMEM and PSTR where the compiler always
 * generates warnings.
 */
#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 
#undef PSTR 
#define PSTR(s) (__ex tension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 

#include <WiFlyHQ.h>


#define greenPin 10    
#define whitePin 13 
#define bluePin 11 
#define redPin 9 
#define steptime 1


//#include <AltSoftSerial.h>
//AltSoftSerial wifiSerial(8,9);

WiFly wifly;

/* Change these to match your WiFi network */
const char mySSID[] = "ssid";
const char myPassword[] = "passwordyou";

void sendIndex();
void sendGreeting(char *name);
void send404();

char buf[80];

#define DEBUG 0
#define FLOAT_SIZE 3
  
#include "math.h"
#define DEG_TO_RAD(X) (M_PI*(X)/180)



struct HSI {
  float h;
  float s;
  float i;
  float htarget;
  float starget;
  int pause;
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

void setup()
{
  color.h = 0;
  color.s = 1;
  color.i = 1;
  color.htarget = 0;
  color.starget = 1;
  color.pause = 0;
  
    pinMode(whitePin, OUTPUT);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
  
  
  if (DEBUG){
    Serial.begin(115200);
    Serial.println(F("Starting"));
    Serial.print(F("Free memory: "));
    Serial.println(wifly.getFreeMemory(),DEC);
  }
    Serial1.begin(115200);

    if (DEBUG){ 
      if (!wifly.begin(&Serial1, &Serial)) {
          Serial.println(F("Failed to start wifly"));
  	wifly.terminal();
      }
    }
    else{
      if (!wifly.begin(&Serial1)) {
  	wifly.terminal();
      }
    }
    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
        turnRed();
	/* Setup the WiFly to connect to a wifi network */
	if (DEBUG)
          Serial.println(F("Joining network"));
	wifly.setSSID(mySSID);
	wifly.setPassphrase(myPassword);
	wifly.enableDHCP();
	wifly.save();

	if (wifly.join()) {
            flashBlue();
            if (DEBUG)
	      Serial.println(F("Joined wifi network"));
	} else {
            if (DEBUG)
	      Serial.println(F("Failed to join wifi network"));
	    wifly.terminal();
	}
    } else {
      flashBlue();
      if (DEBUG)
        Serial.println(F("Already joined network"));
    }

    wifly.setBroadcastInterval(0);	// Turn off UPD broadcast

    //wifly.terminal();
    if (DEBUG) {
      Serial.print(F("MAC: "));
      Serial.println(wifly.getMAC(buf, sizeof(buf)));
      Serial.print(F("IP: "));
      Serial.println(wifly.getIP(buf, sizeof(buf)));
    }
    wifly.setDeviceID("Wifly-WebServer");

    if (wifly.isConnected()) {
      if (DEBUG)
        Serial.println(F("Old connection active. Closing"));
      wifly.close();
    }

    wifly.setProtocol(WIFLY_PROTOCOL_TCP);
    if (wifly.getPort() != 80) {
        wifly.setPort(80);
	/* local port does not take effect until the WiFly has rebooted (2.32) */
	wifly.save();
	if (DEBUG)
          Serial.println(F("Set port to 80, rebooting to make it work"));
	wifly.reboot();
	delay(3000);
    }
    if (DEBUG)
      Serial.println(F("Ready"));
}

void loop()
{
    if (!color.pause){
      //color.h+=1;
      //delay(steptime);
      sendcolor();
    }
  
    if (wifly.available() > 0) {

        /* See if there is a request */
	if (wifly.gets(buf, sizeof(buf))) {
	    if (strncmp_P(buf, PSTR("GET / "), 6) == 0) {
		/* GET request */
                if (DEBUG) Serial.println(F("Got GET request"));
		while (wifly.gets(buf, sizeof(buf)) > 0) {
		    /* Skip rest of request */
		}
		//sendIndex();
                sendGreeting("User");
		if (DEBUG) Serial.println(F("Sent index page"));
	    } else if (strncmp_P(buf, PSTR("POST"), 4) == 0) {
	        /* Form POST */
	        char username[16];
	        //char color[16];
	        if (DEBUG) Serial.println(F("Got POST"));

		/* Get posted field value */
                int match = -1;
               
                do {
                  //wifly.gets(color,FLOAT_SIZE);
                  switch(match) {
                    case 0:
                      color.h=(float)wifly.parseInt();
                      break;
                    case 1:
                      color.s=wifly.parseFloat();
                      break;
                    case 2:
                      color.i=wifly.parseFloat();
                      break;
                    default:
                      break;
                  }
                  match = wifly.multiMatch_P(100,3,
                    F("hue="), F("saturation="), F("intensity="));
                } while (match != -1);
                
                wifly.flushRx();		// discard rest of input
                sendGreeting("User");
	    } else {
	        /* Unexpected request */
		if (DEBUG)
                {  
                  Serial.print(F("Unexpected: "));
		  Serial.println(buf);
		  Serial.println(F("Sending 404"));
		}
                wifly.flushRx();		// discard rest of input
		send404();
	    }
	}
    }
}

/** Send an index HTML page with an input box for a username */
void sendIndex()
{
    /* Send the header direclty with print */
    wifly.println(F("HTTP/1.1 200 OK"));
    wifly.println(F("Content-Type: text/html"));
    wifly.println(F("Transfer-Encoding: chunked"));
    wifly.println();

    /* Send the body using the chunked protocol so the client knows when
     * the message is finished.
     * Note: we're not simply doing a close() because in version 2.32
     * firmware the close() does not work for client TCP streams.
     */
    wifly.sendChunkln(F("<html>"));
    wifly.sendChunkln(F("<title>Saikoduino: Thanks WiFlyHQ!</title>"));
    wifly.sendChunkln(F("<h1>"));
    wifly.sendChunkln(F("<p>Hello</p>"));
    wifly.sendChunkln(F("</h1>"));
    wifly.sendChunkln(F("<form name=\"input\" action=\"/\" method=\"post\">"));
    wifly.sendChunkln(F("Username:"));
    wifly.sendChunkln(F("<input type=\"text\" name=\"user\" />"));
    wifly.sendChunkln(F("<input type=\"submit\" value=\"Submit\" />"));
    wifly.sendChunkln(F("</form>")); 
    wifly.sendChunkln(F("</html>"));
    wifly.sendChunkln();
}


void webChange()//char *colorName)
{
  color.h+=10;
}
  
/** Send a greeting HTML page with the user's name and an analog reading */
void sendGreeting(char *name)
{
    webChange();

    /* Send the header directly with print */
    wifly.println(F("HTTP/1.1 200 OK"));
    wifly.println(F("Content-Type: text/html"));
    wifly.println(F("Transfer-Encoding: chunked"));
    wifly.println();

    /* Send the body using the chunked protocol so the client knows when
     * the message is finished.
     */
    wifly.sendChunkln(F("<html><head>"));
    //wifly.sendChunkln(F("<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.3/jquery.min.js\"></script>"));
    wifly.sendChunkln(F("<style>"));
    wifly.sendChunkln(F(".slider span{width:50px;float:left;} .slider input{width:250px;margin-left:75px;}"));
    wifly.sendChunkln(F(".submit{margin-top:30px;margin-left:50px;width:300px;height:80px;font-size:20pt;}"));
    wifly.sendChunkln(F("body{width:400px;height:300px;margin:100 auto;padding:20px;border:1px dashed grey;}"));
    wifly.sendChunkln(F("</style>"));
    wifly.sendChunkln(F("<title>Saikoduino Control: Thanks WiFlyHQ!</title></head>"));
    /* No newlines on the next parts */
    wifly.sendChunk(F("<h1><p>Hello "));
    wifly.sendChunk(name);
    /* Finish the paragraph and heading */
    wifly.sendChunkln(F("</p></h1>"));

    wifly.sendChunkln(F("<form name=\"input\" action=\"/\" method=\"post\">"));
    /*wifly.sendChunk(F("<input type=\"hidden\" name=\"user\" value=\""));
    wifly.sendChunk(name);
    wifly.sendChunkln(F("\"/>"));*/
    
    if (color.pause)
      wifly.sendChunkln(F("<div>*PAUSED*</div>"));


    char buf[10];

    wifly.sendChunk(F("<div class='slider'><span>Hue</span><input type=\"range\" name=\"hue\" min=0 max=360 step=1 value=\""));
    sprintf(buf,"%d",(int)color.h);//    dtostrf(color.h,FLOAT_SIZE,0,buf);
    wifly.sendChunk(buf);
    wifly.sendChunkln(F("\"/></div>"));
    
    wifly.sendChunk(F("<div class='slider'><span>Saturation</span><input type=\"range\" name=\"saturation\" min=0 max=1 step=.01 value=\""));
    dtostrf(color.s,FLOAT_SIZE,2,buf);
    wifly.sendChunk(buf);
    wifly.sendChunkln(F("\"/></div>"));
    
    wifly.sendChunk(F("<div class='slider'><span>Intensity</span><input type=\"range\" name=\"intensity\" min=0 max=1 step=.01 value=\""));
    dtostrf(color.i,FLOAT_SIZE,2,buf);
    wifly.sendChunk(buf);
    wifly.sendChunkln(F("\"/></div>"));
    
    
    

    wifly.sendChunkln(F("<input class=\"submit\" type=\"submit\" value=\"Update\" />"));
    wifly.sendChunkln(F("</form>")); 

    /* Include a reading from Analog pin 0 
    snprintf_P(buf, sizeof(buf), PSTR("<p>Analog0=%d</p>"), analogRead(A0));

    wifly.sendChunkln(buf);
*/
    wifly.sendChunkln(F("</html>"));
    wifly.sendChunkln();
}

/** Send a 404 error */
void send404()
{
    wifly.println(F("HTTP/1.1 404 Not Found"));
    wifly.println(F("Content-Type: text/html"));
    wifly.println(F("Transfer-Encoding: chunked"));
    wifly.println();
    wifly.sendChunkln(F("<html><head>"));
    wifly.sendChunkln(F("<title>404 Not Found</title>"));
    wifly.sendChunkln(F("</head><body>"));
    wifly.sendChunkln(F("<h1>Not Found</h1>"));
    wifly.sendChunkln(F("<hr>"));
    wifly.sendChunkln(F("</body></html>"));
    wifly.sendChunkln();
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
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*255*I/3*(1+cos_h/cos_1047_h);
    b = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*255*I/3*(1+cos_h/cos_1047_h);
    r = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
  }
  w = (int)(255.0*(1-S)*I);
  
  rgbw[0]=r;
  rgbw[1]=g;
  rgbw[2]=b;
  rgbw[3]=w;
}

void turnRed(){
   analogWrite(redPin,127);
   delay(500);
}

void flashBlue(){
   analogWrite(redPin,0);
   analogWrite(bluePin,127);
   delay(500);
   analogWrite(bluePin,0);
}



               
