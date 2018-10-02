/*
  Teensy_Commands_USB2000_SD_OneFile
  
  20181002
  char headerScan[22]; was 20 and I got a freeze on writing it to file
  merged from Teensy_Commands_USB2000_SD_OneFile_Exp2
  
  20180925
  Added scans on PARK and START
  Added quality calculation

  20180904
  Generate an entire file scan
  Wait for a button press
  Scan is in Loop
  It uses RTC for filename generation and spectrum header


  TODO
  Calculate Park steps!
  Add Light, temp and UV sensors?

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

#define PARKSTEPS 0
#define STARTSTEPS 3000

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
char headerScan[22];

const uint16_t numCharsToReceive = 4108; //vengono scartati i primi 2 FF FF e gli ultimi 2 FF FD
byte tempBuffer[numCharsToReceive];

bool IsGood = false;

uint16_t ActualSteps = PARKSTEPS;
byte NumScans = 50;   //------------------------------------NUMSCANS!!!!!!!!!!!!!!<--------------


//variables for scan evaluation
//Dark
const uint16_t DarkStartIndex = 0;
const uint16_t DarkStopIndex = 33;
float meanDark = 0;
//309 band
const uint16_t B309StartIndex = 279;
const uint16_t B309StopIndex = 289;
float mean309 = 0;
//335 band
const uint16_t B335StartIndex = 620;
const uint16_t B335StopIndex = 630;
float mean335 = 0;
float qualityIndex = 0;
float GlobalQualityIndex = 0;

//Files to send
const uint8_t MaxLenghtFilename = 28;
struct FileScore {
  char filename [MaxLenghtFilename];
  uint16_t score;
};
const uint8_t MaxFilesToSend = 255;
FileScore my2dArray[MaxFilesToSend];
uint8_t my2dArrayIndex = 0;

//exposition
const float CoeffHighOtpimalLevel = 0.9;
const float CoeffLowOtpimalLevel  = 0.8;
const uint16_t MaxIntegrationTime = 2000;
uint16_t IntegrationTime = 100;
uint16_t MaxCounts = 65535;
uint16_t HighOptimalLevel = MaxCounts * CoeffHighOtpimalLevel;
uint16_t LowOptimalLevel = MaxCounts * CoeffLowOtpimalLevel;
uint16_t spectraMax = 0;
uint16_t spectraMin = 65535;
uint16_t GspectraMax = 0;
uint16_t GspectraMin = 65535;
byte CoAdd = 1;

//SDfat
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
float CalculateQuality() {
  //
  float ThisQualityIndex = 0;
  meanDark = 0;
  mean309 = 0;
  mean335 = 0;
  for (uint16_t i = DarkStartIndex ; i <= DarkStopIndex ; i++) {
    meanDark = meanDark + tempBuffer[i];
  }

  for (uint16_t i = B309StartIndex ; i <= B309StopIndex ; i++) {
    mean309 = mean309 + tempBuffer[i];
  }

  for (uint16_t i = B335StartIndex ; i <= B335StopIndex ; i++) {
    mean335 = mean335 + tempBuffer[i];
  }
  meanDark = meanDark / (DarkStopIndex - DarkStartIndex);
  mean309 = mean309 / (B309StopIndex - B309StartIndex);
  mean335 = mean335 / (B335StopIndex - B335StartIndex);
  DEBUG.print("\nQuality Index ---> ");
  DEBUG.print(meanDark);
  DEBUG.print(" ");
  DEBUG.print(meanDark);
  DEBUG.print(" ");
  DEBUG.print(meanDark);
  DEBUG.print(" ");
  ThisQualityIndex = (mean335 - meanDark) / (mean309 - meanDark);
  DEBUG.println(ThisQualityIndex);
  return ThisQualityIndex;
}
//*********************************************************
void ErrorBlink10() {
  for (uint8_t i = 0; i < 11; i++) {
    Blink(100);
  }
}
//*********************************************************
bool FullSkyScan() {
  //Perform a full sky scan starting from park

  // bool AllOk = false;
  GlobalQualityIndex = 0;


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
    //sd.initErrorHalt("SdFatSdio begin() failed");
    //return false;
    ErrorBlink10();
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
  Blink(100);

  //Opening file
  DEBUG.println("opening");
  if (!file.open(filename, O_RDWR | O_CREAT)) {
    DEBUG.println("open failed");
    //sd.errorHalt("open failed");
    DEBUG.println("open file failed!!!");
    //return false;
    ErrorBlink10();
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
  Blink(100);
  DEBUG.println("printing header1");


  file.println(header1);
  file.println(headerFinta1);
  file.println(header2);

  DEBUG.println("printed header1");
  file.flush();
  Blink(100);

  //Goto PARK
  WaitMillis = LongWait / 2;
  COMMAND.println("PARK");
  DEBUG.println("PARK");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on PARK"));
    //return false;
  }
  Blink(100);
  //Get a single scan
  ActualSteps = PARKSTEPS;
  GetScanFromSpectrometer();
  sprintf(headerScan, "%05u %02u %02u %02u %05u", 0, hour(), minute(), second(), ActualSteps / 2);
  DEBUG.println(headerScan);
  file.print(headerScan);
  DEBUG.println("Header written");
  //file.flush();
  //DEBUG.println("flushed2");
  DEBUG.println(numCharsToReceive);
  file.write(tempBuffer, numCharsToReceive);
  file.println("");
  file.flush();
  DEBUG.println("Done PARK scan");

  Blink(100);

  //Goto START
  WaitMillis = LongWait / 2;
  COMMAND.println("START");
  DEBUG.println("START");
  WaitForAnswer();
  if (CheckAnswer() == false) {
    DEBUG.println(F("Failed on START"));
    //return false;
  }
  //Get a single scan
  ActualSteps = STARTSTEPS;
  GetScanFromSpectrometer();
  sprintf(headerScan, "%05u %02u %02u %02u %05u", 1, hour(), minute(), second(), ActualSteps / 2);
  DEBUG.println(headerScan);
  file.print(headerScan);
  DEBUG.println("Header written");
  file.flush();
  DEBUG.println("flushed2");
  DEBUG.println(numCharsToReceive);
  file.write(tempBuffer, numCharsToReceive);
  file.println("");
  file.flush();
  DEBUG.println("Done START scan");

  Blink(100);

  //calculating quality index
  qualityIndex = CalculateQuality();

  //SCAN the others positions
  //Here start the scan cycle ++++++++++++++++++++++++
  WaitMillis = ShortWait;
  for (byte i = 0; i < NumScans; i++) {
    Blink(100);
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
      //return false;
    }
    //Get a single scan
    ActualSteps = ActualSteps + 120;
    GetScanFromSpectrometer();

    DEBUG.println("Writing...");
    sprintf(headerScan, "%05u %02u %02u %02u %05u", i + 2, hour(), minute(), second(),  ActualSteps / 2);
    //i + 2 because we have written 2 scans
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
    //calculating quality index
    qualityIndex = CalculateQuality();
  }
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++
  //Here finish the scan cycle

  DEBUG.println("Closing file");
  file.close();
  DEBUG.println("File full written!");
  zeroesDataArray();

  //Adding file to send list
  //my2dArray[my2dArrayIndex].filename = filename;
  strcpy(my2dArray[my2dArrayIndex].filename, filename );
  my2dArray[my2dArrayIndex].score = qualityIndex;
  return true;
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

  Blink(100);

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

  Blink(100);

  //get a full scan
  IsGood = FullSkyScan();

  if (ONLAB) digitalWrite(ONBOARD_LED, HIGH);

  GotoPark();

  //turn off
  digitalWrite(PowerDevices_SW3, LOW);
  Blink(500);
  DEBUG.println("Stop operations");
}
