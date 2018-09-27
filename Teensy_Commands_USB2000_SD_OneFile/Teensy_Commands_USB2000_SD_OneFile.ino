/*
  Teensy_Commands_USB2000_SD_OneFile
  20180904
  Generate an entire file scan
  Wait for a button press
  Scan is in Loop
  It uses RTC for filename generation and spectrum header


  TODO
  spectrum angle generation
  spectrum quality evaluation

	Commands
  sCmd.addCommand("INIT", Init);
  sCmd.addCommand("PARK", Park);
  sCmd.addCommand("START", Start);
  sCmd.addCommand("NEXT", Next);
  sCmd.addCommand("STOP", Stop);
  sCmd.addCommand("ZENITH", Zenith);
  sCmd.addCommand("GETPOS", GetPos);
  sCmd.addCommand("PRINTVARS", PrintVars);
  sCmd.addCommand("ZTYPE", SlitOrPin);
  sCmd.addCommand("HELLO", Hello);
  sCmd.addCommand("GOTO", Goto);
  sCmd.addCommand("STEPS", MakeSteps);

  When on field comment out
  while (!DEBUG) otherwise it will wait for open COM on USB DEBUG
  change timeouts
  scans should be 50

  Hardware
  29 PushButton

*/

#include <SdFat.h>  //1.0.7
#include <TimeLib.h>

#define BAUDDEBUG 115200
#define BAUDUSB2000 115200
#define BAUDCOMMAND 9600

#define DEBUG Serial
#define COMMAND Serial3 //Serial3 on PCB label but sometimes is 2!!!
#define USB2000 Serial1

//Onboard LED
#define ONBOARD_LED LED_BUILTIN

//Button
#define ONBOARD_BUTTON 29 //on Teensy
#define BUTTON_DEBOUNCE_DELAY 5

const bool ONLAB = false;
const char versione[] = "V.0.6";

const byte TASTO = 29 ;
const byte pressedLevel = LOW;

//recvWithEndMarker
const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

byte NumScans = 10;

//Arduino
char OK1[] = "OK";
char ERR1[] = "ERR";
byte n = 99;

//IO that controls power
const byte PowerDevices_SW3 = 11;

//Serial timeouts
const uint32_t LongWait = 100000; // 100000; //portare a 100 000
const uint32_t ShortWait = 2000; //2000;
uint32_t WaitMillis = LongWait;

uint32_t StartTime;

bool Timeout = false;

const char header1[] = "Vbatt. Vpanel IBatt. Temp.";
const char headerFinta1[] = "13,063 13,1689 0,2253 0,5";
const char header2[] = "N_Acq Hour Min Sec MotorPos CoAdding Int_Time Ch_Num Scan_Numb Scan_MemInt_Time_Count Pixel_Mode Pixel_Mode_Param ---> Spectral_Data...";
char headerScan[20];

const uint16_t numCharsToReceive = 4108; //vengono scartati i primi 2 FF FF e gli ultimi 2 FF FD
byte tempBuffer[numCharsToReceive];

bool IsGood = false;

SdFatSdio sd;
File file;
//*********************************************************
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}
//*********************************************************
void recvWithEndMarker() {
  //https://forum.arduino.cc/index.php?topic=396450.0
  //Timeout added
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
uint8_t isButtonPressed() {
  if (digitalRead(TASTO) == pressedLevel) {
    delay(BUTTON_DEBOUNCE_DELAY);
    while (digitalRead(TASTO) == pressedLevel) ;
    return true;
  }
  return false;
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
void zeroesDataArray() {
  //zeroes spectrum data array
  for (uint16_t i = 0; i < numCharsToReceive; i++) {
    tempBuffer[i] = 0;
  }
}
//*********************************************************
bool CheckAnswer() {
  //Check if the answer from Arduino unit is OK or ERR
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
void GotoPark() {
  //Goto PARK
  WaitMillis = LongWait / 2;
  COMMAND.println("PARK");
  DEBUG.println("PARK");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on PARK"));
  }
}
//*********************************************************
void Blink(uint16_t Delay) {
  digitalWrite(ONBOARD_LED, HIGH);
  delay(Delay);
  digitalWrite(ONBOARD_LED, LOW);
}
//*********************************************************
bool FullSkyScan() {
  //Perform a full sky scan
  bool AllOk = false;

  //ADD goto PArk and get a scan!!!!

  //Goto START
  WaitMillis = LongWait / 2;
  COMMAND.println("START");
  DEBUG.println("START");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on START"));
  }

  //Generate filename
  DEBUG.println("");
  char filename[28] = "Temp.bin";
  sprintf(filename, "%02u%02u%02u_%02u%02u%02u_STRU_Block.txt", year() - 2000, month(), day(), hour(), minute(), second());
  DEBUG.println(filename);

  //Access SD
  DEBUG.println("chvol");
  sd.chvol();
  DEBUG.println("sdbegin");

  if (!sd.begin()) {
    DEBUG.println("SdFatSdio begin() failed");
    //return AllOk
    sd.initErrorHalt("SdFatSdio begin() failed");
  }

  /*
    char filename[] = "Test00.bin";
    for (uint8_t i = 0; i < 100; i++) {
      filename[4] = i / 10 + '0';
      filename[5] = i % 10 + '0';
      if (! sd.exists(filename)) {
        DEBUG.println("opening");
        if (!file.open(filename, O_RDWR | O_CREAT)) {
          DEBUG.println("open failed");
          sd.errorHalt("open failed");
        }
        break; // leave the loop!
      }
    }
  */

  //Opening file
  DEBUG.println("opening");
  if (!file.open(filename, O_RDWR | O_CREAT)) {
    DEBUG.println("open failed");
    //return AllOk
    sd.errorHalt("open failed");
  }
  DEBUG.println("File opened!");
  /*
    //Put timestamp on file
    if (!file.timestamp(T_CREATE, year(), month(), day(), hour(), minute(), second()))
    {
      DEBUG.println("set create time failed");
    }
  */
  //Print first headers

  DEBUG.println("printing header1");


  file.println(header1);
  file.println(headerFinta1);
  file.println(header2);

  DEBUG.println("printed header1");
  file.flush();
  //SCAN
  WaitMillis = ShortWait;
  for (byte i = 0; i < NumScans; i++) {

    //This is spectrum number i
    DEBUG.print(i);
    DEBUG.print(" ");
    //Move head
    COMMAND.println("NEXT");
    DEBUG.println("NEXT");

    //Wait for OK
    WaitForAnswer();
    if (CheckAnswer() == false) {
      DEBUG.println(F("Failed on NEXT"));
      //return AllOk
    }

    GetScanFromSpectrometer();

    DEBUG.println("flushing..");
    file.flush();

    DEBUG.println("Writing...");
    sprintf(headerScan, "%05u %02u %02u %02u %05u", i, hour(), minute(), second(), 159);
    DEBUG.println(headerScan);
    file.print(headerScan);
    DEBUG.println("Header written");
    file.flush();
    DEBUG.println("flushed2");
    DEBUG.println(numCharsToReceive);
    file.write(tempBuffer, numCharsToReceive);
    file.println("");
    file.flush();
    DEBUG.println("Done");
  }

  DEBUG.println("Closing file");
  file.close();
  DEBUG.println("File full written!");
  AllOk = true;
  zeroesDataArray();
  return AllOk;
}
//*********************************************************
//*********************SETUP*******************************
void setup() {
  COMMAND.begin(BAUDCOMMAND);
  DEBUG.begin(BAUDDEBUG);
  USB2000.begin(BAUDUSB2000);

  /*
    while (!DEBUG) {
      SysCall::yield();
      //wait for PC debug USB connection
      //THIS MUST BE ERASED OR COMMENTED OUT WHEN ON FIELD!!!!!!!
    }
  */
  DEBUG.println(F("Teensy_Commands_USB2000_SD_OneFile"));
  DEBUG.println(versione);
  // I/O setup
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(TASTO, INPUT_PULLUP);
  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(PowerDevices_SW3, OUTPUT);
  digitalWrite(PowerDevices_SW3, LOW);

  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);

  delay(100);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }

  char filename[20];
  sprintf(filename, "%02u%02u%02u %02u%02u%02u", year(), month(), day(), hour(), minute(), second());
  DEBUG.println(filename);
  Blink(500);
}
//*********************************************************
//********************LOOP*********************************
void loop() {
  Blink(500);
  DEBUG.println("Waiting for button");

  while (digitalRead(TASTO) != pressedLevel) {
  }

  Blink(500);

  //turn on
  digitalWrite(PowerDevices_SW3, HIGH);
  Blink(500);
  DEBUG.println("Start operations");

  //HELLO
  WaitMillis = ShortWait;
  COMMAND.println("HELLO");
  DEBUG.println("HELLO");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on HELLO"));
  }

  //INIT
  //This should be done once a day
  WaitMillis = LongWait;
  //During Init the remote unit search for zero
  //so it can take the time for a entire round
  COMMAND.println("INIT");
  DEBUG.println("INIT");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on INIT"));
  }

  //get a full scan
  IsGood = FullSkyScan();

  if (ONLAB) digitalWrite(ONBOARD_LED, HIGH);

  GotoPark();

  //turn off
  digitalWrite(PowerDevices_SW3, LOW);
  Blink(500);
  DEBUG.println("Stop operations");
}
