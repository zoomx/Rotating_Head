/*
  StepperCommandsTest




*/

#define DEBUG Serial   //debug
#define COMMAND debug   //debug

#include <SoftwareSerial.h>


const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

SoftwareSerial debug(8, 9); // RX, TX

char OK1[] = "OK";
char ERR1[] = "ERR";
byte n = 99;

//*********************************************************
void recvWithEndMarker() {
  //https://forum.arduino.cc/index.php?topic=396450.0
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (COMMAND.available() > 0 && newData == false) {
    rc = COMMAND.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx - 1] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}
//*********************************************************
void WaitForAnswer() {
  do {
    recvWithEndMarker();
  } while (newData == false);

}
//*********************************************************
bool CheckAnswer() {
  bool flag = false;
  n = 99;
  DEBUG.println(receivedChars);
  for (byte i = 0; i < 10; i++) {
    DEBUG.print(receivedChars[i], HEX);
    DEBUG.print(" ");
  }
  DEBUG.println("");

  n = memcmp ( OK1, receivedChars, sizeof(OK1) );
  if (n == 0 ) {
    DEBUG.println("OK received");
    flag = true;
  }
  n = memcmp ( ERR1, receivedChars, sizeof(ERR1) );
  if (n == 0 ) {
    DEBUG.println("ERR received");
  }
  newData = false;
  receivedChars[0] = '\0';
  DEBUG.println("------------------");
  return flag;
  
}

//*********************************************************
//*********************************************************
void setup() {
  COMMAND.begin(9600);
  DEBUG.begin(115200);
  DEBUG.println(F("StepperCommandsTest"));
  //Serial.println(F("StepperCommandsTest"));

  //INIT
  COMMAND.println("INIT");
  DEBUG.println("INIT");
  while (CheckAnswer() == false) {
    WaitForAnswer();
  }
  //get a scan

  //Goto START
  COMMAND.println("START");
  DEBUG.println("START");
  while (CheckAnswer() == false) {
    WaitForAnswer();
  }

  //get a scan

  //SCAN
  for (byte i = 0; i < 50; i++) {
    DEBUG.print(i);
    DEBUG.print(" ");
    COMMAND.println("NEXT");
    DEBUG.println("NEXT");

    while (CheckAnswer() == false) {
      WaitForAnswer();
    }
    //get a scan
  }

  //Goto PARK
  COMMAND.println("PARK");
  DEBUG.println("PARK");
  while (CheckAnswer() == false) {
    WaitForAnswer();
  }
  //get a scan


}
//*********************************************************
//*********************************************************
void loop() {


}
