/*
World's simplest Android App!
blprnt@blprnt.com
Sept 25, 2010
*/
import java.net.*;

// Build a container to hold the current rotation of the box
float boxRotation = 0;
float h,s,i;
color c;
String ip="192.168.22.1";
int port = 80;
float awidth = 1280;
float aheight=800;
DatagramSocket udpPort;
DatagramPacket dp;


void sendDP(float r, float g, float b, float w){
  
  InetAddress server;
  try{
    server = InetAddress.getByName(ip);
    String message = String.format("%.0f,%.0f,%.0f,%.0f\r\n",r,g,b,0.0);
    byte[] data = message.getBytes();
    dp = new DatagramPacket(data, message.length(), server, port);  
    }
   catch(UnknownHostException exception){
     println("Unknown Host");
     return;
   }
   try {
     //if (dp == null)
     udpPort.send(dp);
     println("success");
   }
   catch(IOException exception){
     println("fail send");
    }
}
  
void setup() {
  println("Setup");

  try{    
    udpPort = new DatagramSocket();
  }
  catch(SocketException e){
    println("socket exception in setup");
  }
  // Set the size of the screen (this is not really necessary 
  // in Android mode, but we'll do it anyway)
  
  size((int)awidth,(int)aheight);
  // Turn on smoothing to make everything pretty.
  smooth();
  // Set the fill and stroke color for the box and circle
  fill(255);
  stroke(255);
  // Tell the rectangles to draw from the center point (the default is the TL corner)
  rectMode(CENTER);
  colorMode(HSB,1.0);
}
void mouseMoved(){
  
}
void release(){ //mouseReleased
  c=color(h,s,i);
  sendDP(red(c)*255,green(c)*255,blue(c)*255,(1.0-s)*127);

}
void draw() {
 float xx=mouseX / awidth;
 float yy=1.0-(2*mouseY / aheight);
  
  // Set the background color, which gets more red as we move our finger down the screen.
  if (yy < -.1) {
    h=xx; s=1;i=1.0+yy;
  }
  else if (yy > .1) {
    h=xx;s=1.0-yy;i=1;
  }
  else{
   h=xx;s=1;i=1;
  }
  
  background(h,s,i);
  //print(yy);print(" ");println(i);
     
  // Change our box rotation depending on how our finger has moved right-to-left
  boxRotation += (float) (pmouseX - mouseX)/100;

  c=color(h,s,i);
  sendDP(red(c)*255,green(c)*255,blue(c)*255,(1.0-s)*127);

  // Draw the ball-and-stick
  line(width/2, height/2, mouseX, mouseY);
  ellipse(mouseX, mouseY, 40, 40);
   fill(c);

  // Draw the box
  pushMatrix();
  translate(width/2, height/2);
  rotate(boxRotation);
  rect(0,0, 50, 50);
  //fill(color(h,s,i));
  fill(c);
  
  popMatrix();
}
