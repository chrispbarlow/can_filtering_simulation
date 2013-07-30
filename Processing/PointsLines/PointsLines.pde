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
int hsCount;
PFont font, fontBold;

int FILTERSIZE = 16;

int d = 15;  
int s = 200;
int w = 900;

int[] mapLineEnd = new int[64];
int[] IDs = new int[64];
int i,j;
int testoffset;
int txPointer = 0;
long total = 0;


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
//  {0x187,8,20},
//  {0x188,8,20},
//  {0x189,8,20},
//  {0x18A,8,20},
//  {0x18B,8,20},
//  {0x18C,8,20},
//  {0x18D,8,20},
//  {0x18E,8,20},
//  {0x207,8,20},
//  {0x209,8,20},
//  {0x20B,8,20},
//  {0x20D,8,20},
//  {0x287,8,20},
//  {0x289,8,20},
//  {0x28B,8,20},
//  {0x28D,8,20},
//  {0x307,8,20},
//  {0x309,8,20},
//  {0x30B,8,20},
//  {0x30D,8,20},
//  {0x385,8,20},
//  {0x387,8,20},
//  {0x389,8,20},
//  {0x38B,8,20},
//  {0x38D,8,20},
//  {0x407,8,20},
//  {0x409,8,20},
//  {0x40B,8,20},
//  {0x40D,8,20},
//  {0x38D,8,20},
//  {0x407,8,20},
};  

long[] counters = new long[loggingList.length];
long[] countersTemp = new long[loggingList.length];

int filterSize = 0;

void setup(){
  size(1200, 1000);
  background(0);
  font = loadFont("Consolas-16.vlw");
  fontBold = loadFont("Consolas-Bold-16.vlw");
  textFont(font, 12);
  if(loggingList.length > 64){
    println("Logging list too long: Max 64 IDs. Cannot continue.");
    exit();
  }
  else{
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
}

int standardSpacingY(int mult, int offset){
  return ((d*mult)+(2*d)+offset);
}

void draw(){
  int barLength = 0;
  String strg = "";
  long countersTotal = 0;
  try{
    background(10);
    
    stroke(255);
    fill(20);
    rect((s-((4*d)+110)), 2, s-d-(s-((4*d)+110)), 25+(filterSize*d),10);
    rect(w+d, 2, 250, 2*d+(loggingList.length*d),10);
    
    stroke(255);
    fill(255);
    textFont(fontBold, 14);
    text("Device Mailboxes", (s-((4*d)+100)), (d+4));
    text(" Logging List      Hits", (w+d+3), (d+4));

    
    /* Draws logging list details */
    for(i=0;i<loggingList.length;i++){
      if(i<10){
        strg = (" 0"+i);
      }
      else{
        strg = (" "+str(i));
      }
      if(allRefresh == true){
        counters[i] = countersTemp[i];
        countersTotal += counters[i];
        total = countersTotal;
      }
      stroke(45);
      line(0, standardSpacingY(i,d/2), 1200, standardSpacingY(i,d/2));   
      stroke(255);
      text(strg+": "+hex(loggingList[i][0],3)+"           "+counters[i], (w+d+5), standardSpacingY(i,6));
      line(w, standardSpacingY(i,0), (w+d), standardSpacingY(i,0));
    }
    
    textFont(fontBold, 12);    
    /* Draws device filter information and mapping lines */
    for(i=0;i<filterSize;i++){
      /* Text and leader lines */
     if(i<10){
        strg = "0"+i;
      }
      else{
        strg = str(i);
      }
      text(strg+": "+hex(IDs[i],3), (s-((4*d)+4)), standardSpacingY(i,6));
      stroke(255);
      line(s, standardSpacingY(i,0), (s-d), standardSpacingY(i,0));
      
      /* Mapping lines */    
      line(s, standardSpacingY(i,0), w, mapLineEnd[i]);   
    } 

    
    fill(0);
    rect((s-((4*d)+110)), standardSpacingY(56,6), 700, 110, 10);
    fill(255);
    

    switch(status){
    case 0:
      strg = "Offline - Can't see device";
      break;      
    case 1:
      strg = "Device waiting - press 'R' to begin.";
      break;
    case 2:
      strg = "Transmitting logging list: ";

      if(txPointer>4){
        strg += txPointer-3;
        rect((s-((4*d)+100)), standardSpacingY(62,10), 680/(loggingList.length/(txPointer-4)), 6);
      }
      break;
    case 3:
      strg = "Online - press 'R' to reset, 'S' to save hit counts, 'C' to save and close.\nTotal hits: ";
      strg += total;
      break;
    case 4:
      strg = "Connection lost - press 'R' to reset";
    default:
      break; 
    }
    


    textFont(fontBold, 16);
    text("Dynamic Filter Mapping Visualisation", (s-((4*d)+100)), standardSpacingY(57,6));
    textFont(font, 14);
    text("This application displays the CAN mailbox to logging list mapping.", (s-((4*d)+100)), standardSpacingY(59,6));
    text("Logging list is sent to the device on connection", (s-((4*d)+100)), standardSpacingY(60,6));
    textFont(fontBold, 14);
    text("Status: "+strg, (s-((4*d)+100)), standardSpacingY(62,6));
    
    allRefresh = false;  
  }
  catch(Exception e){
    exit();
  }
}

void serialEvent(Serial myPort) {
  int inByte;
  long messageCounter = 0; 
  int loggingListPointer = 0;
  int IDhPointer,IDlPointer,lineEndPointer, txListPointer;
    
  try{

    if(serialCount == 0){
      for(j=0;j<serialInArray.length;j++){
        serialInArray[j] = 0;
      }
    }
    
    /* read a byte from the serial port: */
    serialInArray[serialCount] = myPort.read();
    
    /* Device sends '?' character as a handshake / logging list request */
    if(serialInArray[serialCount] == '?'){
      
      /* Prevents '63' values in data stream from being misinterpreted as a handshake request */ 
      if(hsCount < 5){
        hsCount++;
      }
      else{     
        if(serialCount == 0){
           status = 1;
         }
         else{
           status = 4;
         }
         print("HS ");
         redraw();
       }
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
          txPointer++;
        }
        else if(txPointer <= loggingList.length){  
          txListPointer =  txPointer-1;
          
          /* CAN ID high byte */
          myPort.write((loggingList[txListPointer][0]>>8)&0x07);
          /* CAN ID low byte */
          myPort.write (loggingList[txListPointer][0]&0xFF);
          /* Message length in bytes */
          myPort.write (loggingList[txListPointer][1]&0xFF);
          /* Message cycle time */
          myPort.write (loggingList[txListPointer][2]&0xFF);
  
          println(hex((loggingList[txListPointer][0]>>8)&0xFF,1)+" "+hex(loggingList[txListPointer][0]&0xFF,2)+" "+hex(loggingList[txListPointer][1]&0xFF,2)+" "+hex(loggingList[txListPointer][2]&0xFF,2));
          txPointer++;
        }
        else if(txPointer == (loggingList.length+1)){
          myPort.write('~');
          println("~");
          txPointer++;
        }
        else if(txPointer == (loggingList.length+2)){
          myPort.write('}');
          println("}");
          txPointer++;
        }
      }    
      /* End of data packet - update data arrays */
      else if((serialCount>0)&&(serialInArray[serialCount-1] == '~')&&(serialInArray[serialCount] == '}')){
        hsCount = 0;
        status = 3;
        
        /* First character of packet after { indicates packet type */
        switch(serialInArray[1]){ 
             
        case 'M': 
        /* Data packet contains mailbox information */
        
          if(serialCount-3 == 1){
            filterSize = 1;
          }
          else{
            filterSize = (serialCount-3)/3;
          }
          
          for(loggingListPointer=0;loggingListPointer<filterSize;loggingListPointer++){
            IDhPointer = (3*loggingListPointer)+2;
            IDlPointer = (3*loggingListPointer)+3;
            lineEndPointer = (3*loggingListPointer)+4;
            
            IDs[loggingListPointer] = ((serialInArray[IDhPointer]<<8) | serialInArray[IDlPointer]);
            mapLineEnd[loggingListPointer] = standardSpacingY(serialInArray[lineEndPointer],0);
          }
          break;
                 
        case 'S':
        /* Data packet contains loggingList information */ 
         
          loggingListPointer = serialInArray[2];
          println(loggingListPointer);
          
          if(loggingListPointer < loggingList.length){
            /* Unpack 32 bit counter */
            messageCounter  = ((serialInArray[3]&0xFF)<<24);
            messageCounter |= ((serialInArray[4]&0xFF)<<16);
            messageCounter |= ((serialInArray[5]&0xFF)<<8);
            messageCounter |=  (serialInArray[6]&0xFF);
             
            /* counters stored in temp array until refresh required */
            countersTemp[loggingListPointer] = messageCounter;
            
            /* Only refresh loggingList counters on screen when all counters have been received (takes several packets) */
            if(loggingListPointer >= (loggingList.length-1)){
              allRefresh = true;
            }
          }
    
          break;
          
        case '~':
        /* Empty serial packet instructs screen refresh */
          redraw();
          break;
          
        default:        
          break;
        }
                
        serialCount = 0;
      }      
      /* Receiving data packet from TI chip */
      else if((serialInArray[0] == '{') && (serialCount < 999)){
          serialCount++;
      }
    }
    else{
      /* App offline */
      serialCount = 0;
      txPointer = 0;
      
//      lastTime = millis();
//      while (millis()-lastTime < 5);
//      
        myPort.write('?');
//   //   println("connecting");
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
        mapLineEnd[k] = standardSpacingY(k,0);
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
  
  if((key == 'x')||(key == 'X')){
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
  
