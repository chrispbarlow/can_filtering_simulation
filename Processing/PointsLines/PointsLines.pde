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
int status = 0;

PFont font;

int FILTERSIZE = 16;

int d = 15;
int s = 100;
int w = 900;

//int[] mb_y = new int[64];
int[] seq_y = new int[64];
int[] IDs = new int[64];
int i,j;
int testoffset;
int txPointer = 0;

boolean allRefresh = false;

int[][] loggingList = {
  {0x187,8,20},
  {0x188,8,20},
  {0x189,8,20},
  {0x18A,8,20},
  {0x18B,8,20},
  {0x18C,8,20},
  {0x18D,8,20},
  {0x18E,8,20},
  {0x207,8,20},
  {0x209,8,20},
  {0x20B,8,20},
  {0x20D,8,20},
  {0x287,8,20},
  {0x289,8,20},
  {0x28B,8,20},
  {0x28D,8,20},
  {0x307,8,20},
  {0x309,8,20},
  {0x30B,8,20},
  {0x30D,8,20},
  {0x385,8,20},
  {0x387,8,20},
  {0x389,8,20},
  {0x38B,8,20},
  {0x38D,8,20},
  {0x407,8,20},
  {0x409,8,20},
  {0x40B,8,20},
  {0x40D,8,20},
  {0x707,1,100},
  {0x709,1,100},
  {0x70B,1,100},
  {0x70D,1,100},  
  {0x187,8,20},
  {0x188,8,20},
  {0x189,8,20},
  {0x18A,8,20},
  {0x18B,8,20},
  {0x18C,8,20},
  {0x18D,8,20},
  {0x18E,8,20},
  {0x207,8,20},
  {0x209,8,20},
  {0x20B,8,20},
  {0x20D,8,20},
  {0x287,8,20},
  {0x289,8,20},
  {0x28B,8,20},
  {0x28D,8,20},
  {0x307,8,20},
  {0x309,8,20},
  {0x30B,8,20},
  {0x30D,8,20},
  {0x385,8,20},
  {0x387,8,20},
  {0x389,8,20},
  {0x38B,8,20},
  {0x38D,8,20},
  {0x407,8,20},
  {0x409,8,20},
  {0x40B,8,20},
  {0x40D,8,20},
  {0x707,1,100},
  {0x709,1,100},
//  {0x70B,1,100},
//  {0x70D,1,100},
};  

long[] counters = new long[loggingList.length];
long[] countersTemp = new long[loggingList.length];

int filterSize = 0;

void setup(){
  size(1200, 1000);
  background(0);
  font = loadFont("Consolas-16.vlw");
  textFont(font, 12);
  
  /* Serial port will be Serial.list()[0] when nothing else connected */
  try{
    println(Serial.list());
    String portName = Serial.list()[0];
    myPort = new Serial(this, portName, 9600);
  }
  catch(Exception e){
    /* App will close if no serial ports are found */
    println("No serial ports found. Use your dongle!");
    exit();
  }
  
  stroke(153);    
  noLoop();
}

void draw(){
  String strg = "";
  try{
    background(0);
    stroke(255);
    text("Filter in Device", (s-((4*d)+4)), (d+6));
    text(" Logging List      Hits", (w+d+5), (d+6));
  
    /* Draws logging list details */
    stroke(255);
    for(i=0;i<loggingList.length;i++){
      if(i<10){
        strg = (" 0"+i);
      }
      else{
        strg = (" "+str(i));
      }
      if(allRefresh == true){
        counters[i] = countersTemp[i];
      }
      text(strg+": "+hex(loggingList[i][0],3)+"           "+counters[i], (w+d+5), ((d*i)+(2*d)+6));
      line(w, ((d*i)+(2*d)), (w+d), ((d*i)+(2*d)));
    }
  
    /* Draws device filter information and mapping lines */
    for(i=0;i<filterSize;i++){
      /* Text and leader lines */
     if(i<10){
        strg = "0"+i;
      }
      else{
        strg = str(i);
      }
      text(strg+": "+hex(IDs[i],3), (s-((4*d)+4)), ((d*i)+(2*d)+6));
      stroke(255);
      line(s, ((d*i)+(2*d)), (s-d), ((d*i)+(2*d)));
      
      /* Mapping lines */    
      line(s, ((d*i)+(2*d)), w, seq_y[i]);
   
    } 
    
    switch(status){
     case 0:
      strg = "offline";
       break;
      case 1:
        strg = "device waiting - press 'R' to begin";
      break;
      case 2:
        strg = "Transmitting logging list";
        for(i=0;i<txPointer;i++){
          if(i%10 == 0){
            strg += ".";
          }
        }
      break;
      case 3:
        strg = "Online";
      break;
      default:
     break; 
    }
    
    text("Status: "+strg, (s-((4*d)+4)), 981);
  
    allRefresh = false;  
  }
  catch(Exception e){
    exit();
  }
}

void serialEvent(Serial myPort) {
  long messageCounter = 0; 
  int loggingListPointer = 0;
  // read a byte from the serial port:
  int inByte = myPort.read();
    // Add the latest byte from the serial port to array:
  try{
    
    if(serialCount == 0){
      for(j=0;j<1000;j++){
        serialInArray[j] = 0;
      }
    }
    serialInArray[serialCount] = inByte;
    if(serialInArray[serialCount] == '?'){
       status = 1;
       print("HS ");
    }
    
    if(readyState==true){
      /* Request from TI chip to send CAN IDs */
      if((serialCount == 0)&&(serialInArray[0] == '?')){       
        status = 2;
       /* this delay is necesssary to all the TI chip to keep up when it receives erroneous null characters */ 
        lastTime = millis();
        while (millis()-lastTime < 5);
        
        if(txPointer == 0){
          myPort.write('{');
          println("{");
        }
        else if(txPointer <= loggingList.length){      
          myPort.write((loggingList[txPointer-1][0]>>8)&0x07);
          myPort.write(loggingList[txPointer-1][0]&0xFF);
          myPort.write(loggingList[txPointer-1][1]&0xFF);
          myPort.write(loggingList[txPointer-1][2]&0xFF);
  
          println(hex((loggingList[txPointer-1][0]>>8)&0xFF,1)+" "+hex(loggingList[txPointer-1][0]&0xFF,2)+" "+hex(loggingList[txPointer-1][1]&0xFF,2)+" "+hex(loggingList[txPointer-1][2]&0xFF,2));
        }
        else if(txPointer == (loggingList.length+1)){
          myPort.write('~');
          println("~");
        }
        else if(txPointer == (loggingList.length+2)){
          myPort.write('}');
          println("}");
        }
        
        txPointer++;
        redraw();
      }    
      /* End of data packet - update data arrays */
      else if((serialCount>0)&&(serialInArray[serialCount-1] == '~')&&(serialInArray[serialCount] == '}')){
        status = 3;
        switch(serialInArray[1]){ 
        /* Data packet contains mailbox information */     
        case 'M': 
          if(serialCount-3 == 1){
            filterSize = 1;
          }
          else{
            filterSize = (serialCount-3)/3;
          }
  //        println(serialCount+": "+filterSize);
          
          for(loggingListPointer=0;loggingListPointer<filterSize;loggingListPointer++){
                IDs[loggingListPointer] = ((serialInArray[(3*loggingListPointer)+2])<<8)|serialInArray[(3*loggingListPointer)+3];
                seq_y[loggingListPointer] = (d*serialInArray[(3*loggingListPointer)+4])+(2*d);
          }
          break;
          
        /* Data packet contains loggingList information */  
        case 'S':
          loggingListPointer = serialInArray[2];
          println(loggingListPointer);
          
          if(loggingListPointer < loggingList.length){
            messageCounter = ((serialInArray[3]&0xFF)<<24);
            messageCounter |= ((serialInArray[4]&0xFF)<<16);
            messageCounter |= ((serialInArray[5]&0xFF)<<8);
            messageCounter |= ((serialInArray[6])&0xFF);
             
            countersTemp[loggingListPointer] = messageCounter;
            
            /* Only refresh loggingList counters on screen when all counters have been received (takes several packets) */
            if(loggingListPointer == (loggingList.length-1)){
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
  catch(Exception e){
    exit();
  }
}

void keyPressed() { // Press a key to save the data
  
  int k;
  
  if((key == 'r')||(key == 'R')){
     readyState = !readyState;
     println(readyState);
     for(k=0;k<filterSize;k++){
        IDs[k] = 0x000;
        seq_y[k] = (d*k)+(2*d);
     }
     
     for (k = 0; k < loggingList.length; k++) {
      counters[k] = 0;
    }
    filterSize = 0;
    status = 0;
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
  String[] lines = new String[loggingList.length];
  
  for (int k = 0; k < loggingList.length; k++) {
      lines[k] = str(counters[k]);
  }
  saveStrings("log_output.txt", lines);
}
  
