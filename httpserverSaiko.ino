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
#define steptime 0
#define propgain 0.0005 // "Small Constant"
#define minsaturation 0.3
#define maxsaturation 0.6

//#include <AltSoftSerial.h>
//AltSoftSerial wifiSerial(8,9);

WiFly wifly;

/* Change these to match your WiFi network */
const char mySSID[] = "evilcorp";
const char myPassword[] = "cockknocker";

void sendIndex();
void sendGreeting();
void send404();

char buf[80];

#define DEBUG 1
#define FLOAT_SIZE 3
  
#include "math.h"
#define DEG_TO_RAD(X) (M_PI*(X)/180)



struct HSI {

  float h;
  float s;
  float i;
  float htarget;
  float starget;
  float h1;
  float h2;
  int mode;
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

typedef enum{SOLID,AUTO,FADE} MODES;

void setup()
{
  color.h = 350;
  color.s = 1;
  color.i = .2;
  color.h1 = 10;
  color.h2 = 180;
  color.mode = AUTO; 
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
      while (!Serial1) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

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
      switch(color.mode){
        case SOLID:
          color.h = color.h1;
          color.s = 1;
          color.i = 1;
          break;
        case AUTO:
          updateHue();
          updateSaturation();
          delay(steptime);
          break;
        case FADE:
          //float next = color.h+color.inc;
          //if color.h > color.h2
          break;
        default:
          break;
      }
      //color.h+=1;
      //
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
                sendGreeting();
		if (DEBUG) Serial.println(F("Sent index page"));
	    } else if (strncmp_P(buf, PSTR("POST"), 4) == 0) {
	        /* Form POST */
	        if (DEBUG) Serial.println(F("Got POST"));
                turnBlack();

		/* Get posted field value */
                int match = -1;
               
                do {
                  //wifly.gets(color,FLOAT_SIZE);
                  switch(match) {
                    case 0:
                      color.mode = wifly.parseInt();
                      break;
                    case 1:
                      color.h=(float)wifly.parseInt();
                      break;
                    case 2:
                      color.s=wifly.parseFloat();
                      break;
                    case 3:
                      color.i=wifly.parseFloat();
                      break;
                    case 4:
                      color.h1=(float)wifly.parseInt();
                      break;
                    case 5:
                      color.h2=(float)wifly.parseInt();
                      break;
                    default:
                      break;
                  }
                  match = wifly.multiMatch_P(100,6,
                    F("mode="), F("hue="), F("saturation="), F("intensity="),F("huestart="),F("hueend="));
                } while (match != -1);
                
                wifly.flushRx();		// discard rest of input
                sendGreeting();
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


/** Send a greeting HTML page with the user's name and an analog reading */
void sendGreeting()
{

    /* Send the header directly with print */
    wifly.println(F("HTTP/1.1 200 OK"));
    wifly.println(F("Content-Type: text/html"));
    wifly.println(F("Transfer-Encoding: chunked"));
    wifly.println();

    /* Send the body using the chunked protocol so the client knows when
     * the message is finished.
     */
    wifly.sendChunkln(F("<html><head>"));
    wifly.sendChunkln(F("<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.3/jquery.min.js\"></script>"));
    
    wifly.sendChunkln(F("<style>"));
      //wifly.sendChunkln(F(".slider span{width:50px;float:left;} .slider input{width:250px;margin-left:75px;}"));
      wifly.sendChunkln(F(".submit{margin-top:30px;margin-left:50px;width:300px;height:40px;font-size:20pt;}"));
      wifly.sendChunkln(F("body{width:400px;height:400px;margin:100 auto;padding:20px;border:1px dashed grey;}"));

      wifly.sendChunkln(F("form{margin-top:20px;}"));
//.hue { width:20px; height: 200px; background: -moz-linear-gradient(top, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%); background: -ms-linear-gradient(top, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%);background: -o-linear-gradient(top, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%); background: -webkit-gradient(linear, left top, left bottom, from(#ff0000), color-stop(0.17, #ffff00), color-stop(0.33, #00ff00), color-stop(0.5, #00ffff), color-stop(0.67, #0000ff), color-stop(0.83, #ff00ff), to(#ff0000));background: -webkit-linear-gradient(top, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%);}
      wifly.sendChunkln(F(".hue { width:348px; height: 20px; margin:50px 10px 10px 60px; background: -moz-linear-gradient(left, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%); background: -ms-linear-gradient(left, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%);background: -o-linear-gradient(left, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%); background: -webkit-gradient(linear, left bottom, right bottom, from(#ff0000), color-stop(0.17, #ffff00), color-stop(0.33, #00ff00), color-stop(0.5, #00ffff), color-stop(0.67, #0000ff), color-stop(0.83, #ff00ff), to(#ff0000));background: -webkit-linear-gradient(left, #ff0000 0%, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000 100%);}"));
      wifly.sendChunkln(F(".hue span {position: relative; left: -75px;}"));
      wifly.sendChunkln(F(".hue input {width: 350px;}"));
      wifly.sendChunkln(F(".center {text-align: center;}"));
      wifly.sendChunkln(F(".options {margin:0 auto;  }"));
      wifly.sendChunkln(F(".options label{width:130px;text-align:center;display:inline-block;}"));


    wifly.sendChunkln(F("</style>"));
    
    wifly.sendChunkln(F("<title>Saikoduino Hacktastic!</title></head>"));
    /* No newlines on the next parts */
    
    wifly.sendChunkln(F("<div class='center'>"));
      wifly.sendChunkln(F("<h1>SaikoLED!</h1>"));
    
      if (color.mode == SOLID)
        wifly.sendChunkln(F("<h2>Solid Color</h2>"));
      else if (color.mode == AUTO)
        wifly.sendChunkln(F("<h2>Auto Fade</h2>"));
      else if (color.mode == FADE)
        wifly.sendChunkln(F("<h2>Cross Fade</h2>"));
      else 
        wifly.sendChunkln(F("<h2>Unknown Mode</h2>"));
      
      if (color.pause)
        wifly.sendChunkln(F("<div>*PAUSED*</div>"));
      else
        wifly.sendChunkln(F("<div>*RUNNING*</div>"));
    wifly.sendChunkln(F("</div>"));
    
    wifly.sendChunkln(F("<form name=\"input\" action=\"/\" method=\"post\">"));
    

    char buf[10];
    
/*
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
  */  
    
    wifly.sendChunkln(F("<script>var hideShow = function(id){console.log(id);return false;}</script>"));
    wifly.sendChunk(F("<div class='options' id='option-list'>"));
    
    wifly.sendChunk(F("<label onclick=\"hideShow('sc');\" for='label-sc'><input type=\"radio\" name=\"mode\" value=\"0\" id='label-sc'"));
    if (color.mode == 0)
      wifly.sendChunk(" checked=\"checked\"");
    wifly.sendChunk(F(">Solid Color</label>"));
    
    wifly.sendChunk(F("<label for='label-af'><input type=\"radio\" name=\"mode\" value=\"1\" id='label-af'"));
    if (color.mode == 1)
      wifly.sendChunk(" checked=\"checked\"");
    wifly.sendChunk(F(">Auto Fade</label>"));
    
    wifly.sendChunk(F("<label for='label-cf'><input type=\"radio\" name=\"mode\" value=\"2\" id='label-cf'"));
    if (color.mode == 2)
      wifly.sendChunk(" checked=\"checked\"");
    wifly.sendChunkln(F(">Cross Fade</label></div>"));

    wifly.sendChunk(F("<div class='hue'><span>Start Color</span><input type=\"range\" name=\"huestart\" min=0 max=360 step=1 value=\""));
    sprintf(buf,"%d",(int)color.h1);//    dtostrf(color.h,FLOAT_SIZE,0,buf);
    wifly.sendChunk(buf);
    wifly.sendChunkln(F("\"/></div>"));
  
    wifly.sendChunk(F("<div class='hue'><span>End Color</span><input type=\"range\" name=\"hueend\" min=0 max=360 step=1 value=\""));
    sprintf(buf,"%d",(int)color.h2);//    dtostrf(color.h,FLOAT_SIZE,0,buf);
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
void turnBlack(){
   analogWrite(redPin,0);
   analogWrite(greenPin,0);
   analogWrite(bluePin,0);
   analogWrite(whitePin,0);
}

void flashBlue(){
   analogWrite(redPin,0);
   analogWrite(bluePin,127);
   delay(500);
   analogWrite(bluePin,0);
}

void updateHue() {
  color.htarget += (random(360)-180)*.1;
  color.htarget = fmod(color.htarget, 360);
  color.h += propgain*(color.htarget-color.h);
  color.h = fmod(color.h, 360);
}

void updateSaturation() {
  color.starget += (random(10000)-5000)/0.00001;
  if (color.starget > maxsaturation) color.starget = maxsaturation;
  else if (color.starget < minsaturation) color.starget = minsaturation;
  color.s += propgain*(color.starget-color.s);
  if (color.s > maxsaturation) color.s = maxsaturation;
  else if (color.s < minsaturation) color.s = minsaturation;
}

               
