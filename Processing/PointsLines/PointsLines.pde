/**
 * Points and Lines. 
 * 
 * Points and lines can be used to draw basic geometry.
 * Change the value of the variable 'd' to scale the form.
 * The four variables set the positions based on the value of 'd'. 
 */
import processing.serial.*;
Serial myPort;                       // The serial port
int[] serialInArray = new int[50];    // Where we'll put what we receive
int serialCount = 0;                 // A count of how many bytes we receive

int d = 20;
int s = 100;
int w = 800;

int[] mb_y = new int[16];
int[] seq_y = new int[16];
int[] counters = new int[33];
int i,j;
int testoffset;

void setup(){
  size(1000, 700);
  background(0);
  for(i=0;i<16;i++){
    mb_y[i] = (d*i)+d;
    seq_y[i] = (d*i)+d;    
  }
  // Print a list of the serial ports, for debugging purposes:
  println(Serial.list());
  String portName = Serial.list()[0];
  myPort = new Serial(this, portName, 9600);
  
  stroke(153);  

  
  noLoop();
}

void draw(){
  background(0);
  stroke(153);
  for(i=0;i<16;i++){
    line(s, mb_y[i], w, seq_y[i]);
    text(i, (s-(2*d)), ((d*i)+d));
    line(s, ((d*i)+d), (s-d), ((d*i)+d));
  }

  for(i=0;i<33;i++){
    text(i, (w+d), ((d*i)+d));
    text(counters[i], (w+(2*d)), ((d*i)+d));
    line(w, ((d*i)+d), (w+d), ((d*i)+d));
  }
}

void serialEvent(Serial myPort) {
  int messageCounter = 0, sequencePointer = 0;
  // read a byte from the serial port:
  int inByte = myPort.read();
    // Add the latest byte from the serial port to array:
  if(serialCount == 0){
    for(j=0;j<50;j++){
      serialInArray[j] = 0;
    }
  }
  
  if(inByte != '~'){
    serialInArray[serialCount] = inByte;
    serialCount++;
  }
  else if((serialInArray[1] == ',')&&(serialInArray[3] == ',')){
    sequencePointer = serialInArray[2];
      
    if(sequencePointer < 33){
      messageCounter = ((serialInArray[4]<<8)|serialInArray[5]);
      counters[sequencePointer] = messageCounter;
      serialCount = 0;
        seq_y[serialInArray[0]] = (d*sequencePointer)+d;
        serialCount = 0;
    }

    redraw();
    // Reset serialCount:
//    for(j=0;j<50;j++){
//      print(serialInArray[j]+" ");
//    }
  }
  else{
    serialCount = 0;
  }
}

