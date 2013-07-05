/**
 * Points and Lines. 
 * 
 * Points and lines can be used to draw basic geometry.
 * Change the value of the variable 'd' to scale the form.
 * The four variables set the positions based on the value of 'd'. 
 */
import processing.serial.*;
Serial myPort;                       // The serial port
int[] serialInArray = new int[1000];    // Where we'll put what we receive
int serialCount = 0;                 // A count of how many bytes we receive
boolean readyState = false;
long lastTime = 0;

int FILTERSIZE = 16;

int d = 20;
int s = 100;
int w = 800;

int[] mb_y = new int[FILTERSIZE];
int[] seq_y = new int[FILTERSIZE];
int[] IDs = new int[FILTERSIZE];
int i,j;
int testoffset;
int txPointer = 0;

boolean allRefresh = false;

int[] sequence = {
  0x187,
  0x188,
  0x189,
  0x18A,
  0x18B,
  0x18C,
  0x18D,
  0x18E,
  0x207,
  0x209,
  0x20B,
  0x20D,
  0x287,
  0x289,
  0x28B,
  0x28D,
  0x307,
  0x309,
  0x30B,
  0x30D,
  0x385,
//  0x387,
//  0x389,
//  0x38B,
//  0x38D,
//  0x407,
//  0x409,
//  0x40B,
//  0x40D,
//  0x707,
//  0x709,
//  0x70B,
//  0x70D
};  

long[] counters = new long[sequence.length];
long[] countersTemp = new long[sequence.length];


void setup(){
  size(1000, 700);
  background(0);
  for(i=0;i<FILTERSIZE;i++){
    mb_y[i] = (d*i)+(2*d);
    seq_y[i] = (d*i)+(2*d);    
  }
  // Print a list of the serial ports, for debugging purposes:
  println(Serial.list());
  String portName = Serial.list()[0];
  myPort = new Serial(this, portName, 9600);
  
  stroke(153);  

  
  noLoop();
}

void draw(){
  String strg;
  background(0);
  stroke(255);
  text("Filter", (s-(4*d)+3), (d+6));
  text("Buffer Hits", (w+d+5), (d+6));

  for(i=0;i<FILTERSIZE;i++){
    stroke(255);
    line(s, mb_y[i], w, seq_y[i]);
    if(i<10){
      strg = "0"+i;
    }
    else{
      strg = str(i);
    }
    text(strg+":   "+hex(IDs[i],3), (s-(4*d)+3), ((d*i)+(2*d)+6));
    stroke(255);
    line(s, ((d*i)+(2*d)), (s-d), ((d*i)+(2*d)));
  }
  
  stroke(255);
  for(i=0;i<sequence.length;i++){
    if(i<10){
      strg = "0"+i;
    }
    else{
      strg = str(i);
    }
    if(allRefresh == true){
      counters[i] = countersTemp[i];
    }
    text(strg+": "+hex(sequence[i],3)+" = "+counters[i], (w+d+5), ((d*i)+(2*d)+6));
    line(w, ((d*i)+(2*d)), (w+d), ((d*i)+(2*d)));
  }
  allRefresh = false;
}

void serialEvent(Serial myPort) {
  long messageCounter = 0; 
  int sequencePointer = 0;
  // read a byte from the serial port:
  int inByte = myPort.read();
    // Add the latest byte from the serial port to array:
  if(serialCount == 0){
    for(j=0;j<1000;j++){
      serialInArray[j] = 0;
    }
  }
  serialInArray[serialCount] = inByte;
    if(serialInArray[serialCount] == '?'){
       print("HS ");
    }
  
  if(readyState==true){
    /* Request from TI chip to send CAN IDs */
    if((serialCount == 0)&&(serialInArray[0] == '?')){
       print("HS ");
     
     /* this delay is necesssary to all the TI chip to keep up when it receives erroneous null characters */ 
      lastTime = millis();
      while (millis()-lastTime < 5);
      
      if(txPointer == 0){
        myPort.write('{');
        println("{");
      }
      else if(txPointer <= sequence.length){      
        myPort.write((sequence[txPointer-1]>>8)&0x07);
        myPort.write(sequence[txPointer-1]&0xFF);
        println(hex((sequence[txPointer-1]>>8)&0xFF,1)+" "+hex(sequence[txPointer-1]&0xFF,2));
      }
      else if(txPointer == (sequence.length+1)){
        myPort.write('~');
        println("~");
      }
      else if(txPointer == (sequence.length+2)){
        myPort.write('}');
        println("}");
      }
      
      txPointer++;
    }    
    /* End of data packet - update data arrays */
    else if((serialCount>0)&&(serialInArray[serialCount-1] == '~')&&(serialInArray[serialCount] == '}')){
      
      switch(serialInArray[1]){ 
      /* Data packet contains mailbox information */     
      case 'M': 
        for(sequencePointer=0;sequencePointer<FILTERSIZE;sequencePointer++){
              IDs[sequencePointer] = ((serialInArray[(3*sequencePointer)+2])<<8)|serialInArray[(3*sequencePointer)+3];
              seq_y[sequencePointer] = (d*serialInArray[(3*sequencePointer)+4])+(2*d);
        }
        break;
        
      /* Data packet contains sequence information */  
      case 'S':
        sequencePointer = serialInArray[2];
        
        if(sequencePointer < sequence.length){
          messageCounter = ((serialInArray[3]&0xFF)<<24);
          messageCounter |= ((serialInArray[4]&0xFF)<<16);
          messageCounter |= ((serialInArray[5]&0xFF)<<8);
          messageCounter |= ((serialInArray[6])&0xFF);
           
          countersTemp[sequencePointer] = messageCounter;
          
          /* Only refresh sequence counters on screen when all counters have been received (takes several packets) */
          if(sequencePointer == (sequence.length-1)){
            allRefresh = true;

          }
        }
  
        break;
        
      case '~':
        redraw();
        break;
        
      default:        
        break;
      }
              
      serialCount = 0;
    }      
    /* Receiving data packet from TI chip */
    else if((serialInArray[0] == '{')&&serialCount < (999)){
        serialCount++;
    }
  }
  else{
    serialCount = 0;
    txPointer = 0;
  }
}

void keyPressed() { // Press a key to save the data
  
  int k;
  
  if((key == 'r')||(key == 'R')){
     readyState = !readyState;
     println(readyState);
     for(k=0;k<FILTERSIZE;k++){
        IDs[k] = 0x000;
        seq_y[k] = (d*k)+(2*d);
     }
     
     for (k = 0; k < sequence.length; k++) {
      counters[k] = 0;
    }

    
    redraw();

  }
  
  if((key == 's')||(key == 'S')){
    saveCounters();
  } 

  if((key == 'c')||(key == 'C')){
    saveCounters();
    exit(); // Stop the program
  } 
  
  if((key == 'f')||(key == 'F')){
    saveFrame("output/frames####.png");
  }
}

void saveCounters(){
  String[] lines = new String[sequence.length];
  
  for (int k = 0; k < sequence.length; k++) {
      lines[k] = str(counters[k]);
  }
  saveStrings("log_output.txt", lines);
}
  
