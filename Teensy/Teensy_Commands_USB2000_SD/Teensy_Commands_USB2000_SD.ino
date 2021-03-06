/*
  Teensy_Commands_USB2000_SD
  20180725

	Commands

*/
#define BAUDDEBUG 115200
#define BAUDUSB2000 115200
#define BAUDCOMMAND 9600

#define DEBUG Serial
#define COMMAND Serial3
#define USB2000 Serial1

//Onboard LED
#define ONBOARD_LED LED_BUILTIN

//Button
#define ONBOARD_BUTTON 68 //on Teensy
#define BUTTON_DEBOUNCE_DELAY 5
const byte TASTO = 68 ;
const byte pressedLevel = LOW;

//recvWithEndMarker
const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

byte NumScans = 5;

//USB2000
char OK1[] = "OK";
char ERR1[] = "ERR";
byte n = 99;

//Serial timeouts
const uint32_t LongWait = 100000;
const uint32_t ShortWait = 2000;
uint32_t WaitMillis = LongWait;

uint32_t StartTime;

bool Timeout = false;

//*********************************************************
void recvWithEndMarker() {
  //https://forum.arduino.cc/index.php?topic=396450.0
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;


  while (COMMAND.available() > 0 && newData == false && millis() - StartTime <= WaitMillis) {
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
  if (millis() - StartTime > WaitMillis) {
    if (ndx > 0) {
      receivedChars[ndx - 1] = '\0'; // terminate the string

    }

    ndx = 0;
    newData = true;
    Timeout = true;
  }
}
//*********************************************************
void WaitForAnswer() {
  Timeout = false;
  DEBUG.print("WaitMillis ->");
  DEBUG.println(WaitMillis);
  StartTime = millis();
  do {
    recvWithEndMarker();
  } while (newData == false);
  if (Timeout) {
    DEBUG.println("Timeout");
    DEBUG.println(millis() - StartTime);
  }
}
//*********************************************************
bool CheckAnswer() {
  bool flag = false;
  if (Timeout) {
    newData = false;
    return flag;
  }
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
  COMMAND.begin(BAUDCOMMAND);
  DEBUG.begin(BAUDDEBUG);
  USB2000.begin(BAUDUSB2000);

  while (!DEBUG) {
    //wait for PC debug USB connection
  }


  DEBUG.println(F("Teensy_Commands_USB2000_SD"));

  // I/O setup
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(TASTO, INPUT_PULLUP);



  //HELLO
  WaitMillis = ShortWait;
  COMMAND.println("HELLO");
  DEBUG.println("HELLO");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on HELLO"));
  }

  //INIT
  WaitMillis = LongWait;
  COMMAND.println("INIT");
  DEBUG.println("INIT");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on INIT"));
  }

  //get a scan

  //Goto START
  WaitMillis = LongWait / 2;
  COMMAND.println("START");
  DEBUG.println("START");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on START"));
  }
  GetScanFromSpectrometer();


  //SCAN
  WaitMillis = ShortWait;
  for (byte i = 0; i < NumScans; i++) {
    DEBUG.print(i);
    DEBUG.print(" ");
    COMMAND.println("NEXT");
    DEBUG.println("NEXT");

    WaitForAnswer();
    if (CheckAnswer() == false) {
      DEBUG.println(F("Failed on NEXT"));
    }
    GetScanFromSpectrometer();
  }

  //Goto PARK
  WaitMillis = LongWait / 2;
  COMMAND.println("PARK");
  DEBUG.println("PARK");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on PARK"));
  }
  //get a scan


}
//*********************************************************
//*********************************************************
void loop() {


}
