/*
  Teensy USB2000 communications

*/


const uint16_t numCharsToReceive = 4112;
const uint8_t SuccessfulMarker = 2;   //STX
const uint8_t UnuccessfulMarker = 3;   //ETX
const uint8_t ACK = 6;
const uint8_t NAK = 21;
const uint32_t WaitMillisUSB2000 = 1000;

byte tempBuffer[numCharsToReceive];

uint16_t bytesRecvd = 0;
uint16_t spectraMax = 0;
uint16_t spectraMin = 65535;

bool inProgress = false;
bool allReceived = false;

#include <SdFat.h>
SdFatSdio sd;
File file;



//*****************************************
//*****************************************
void GetScanFromSpectrometer() {



  USB2000.print("v");
  if (CheckACK() == true) {
    DEBUG.print("Spectrometer version ");
    DEBUG.println(readWord());
  }


  //GetAscan();
  DEBUG.println("Press button to scan");
  delay(500);


  spectraMax = 0;
  spectraMin = 65535;
  GetAscan();

  //write on SD
  static uint32_t ifl = 0;
  static uint32_t count = 0;
  //char filename[32] = "Test.bin";

  DEBUG.println("chvol");
  sd.chvol();
  DEBUG.println("sdbegin");

  if (!sd.begin()) {
    DEBUG.println("SdFatSdio begin() failed");
    sd.initErrorHalt("SdFatSdio begin() failed");
  }

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
  DEBUG.println(filename);

  /*
    DEBUG.println("opening");
    if (!file.open("Test.bin", O_RDWR | O_CREAT)) {
      DEBUG.println("open failed");
      sd.errorHalt("open failed");
    }
  */
  DEBUG.println("File opened!");
  /*
    for (int i = 0; i <= numCharsToReceive ; i++)
    {

    file.write(tempBuffer[i]);
    DEBUG.println(i);
    }

  */
  DEBUG.println("Writing...");
  file.write(tempBuffer, numCharsToReceive);
  DEBUG.println("Flush");
  file.flush();
  DEBUG.println("Closing");
  file.close();
  DEBUG.println("File written!");
}
//*****************************************
uint8_t isButtonPressed() {
  if (digitalRead(TASTO) == pressedLevel) {
    delay(BUTTON_DEBOUNCE_DELAY);
    while (digitalRead(TASTO) == pressedLevel) ;
    return true;
  }
  return false;
}

//*****************************************
void getSerialData() {
  byte x;
  uint32_t StopTime;
  bool firstByte = true;
  uint8_t ulowByte = 0;
  uint8_t uhighByte = 0;
  uint16_t RawValue = 0;
  uint32_t StartTime = millis();


  while (millis() - StartTime <= WaitMillisUSB2000 && allReceived == false) {      //while we have not received all chars
    if (USB2000.available() > 0)      //Are there new data?
    {

      x = USB2000.read();        //Yes!! Read data

      if (inProgress) {               // maybe this can be removed
        tempBuffer[bytesRecvd] = x;
        bytesRecvd ++;
      }

      if (bytesRecvd > 16) {  //Header is 16 bytes, last byte is an end packet
        if (firstByte == true) {
          uhighByte  = x;
          firstByte = false;
          DEBUG.print(uhighByte);
        }
        else {
          ulowByte = x;
          RawValue = ulowByte | uhighByte << 8;
          if (bytesRecvd < 4111) {
            if (RawValue > spectraMax) {
              spectraMax = RawValue;
            }
            if (RawValue < spectraMin) {
              spectraMin = RawValue;
            }
            DEBUG.print(" ");
            DEBUG.print(ulowByte);
            DEBUG.print(" ");
            DEBUG.println(RawValue);
          }
          if (bytesRecvd > 4111) {
            DEBUG.println("");
            DEBUG.println(RawValue);
          }
          firstByte = true;
        }

      }

      if (bytesRecvd == numCharsToReceive) {    // Are all data received? add here check timeout
        inProgress = false;                     //Yes, stop receive data
        allReceived = true;                     //All data received
        StopTime = millis();
      }
    }
  }
  DEBUG.print("got all data in ");
  DEBUG.print (StopTime - StartTime);
  DEBUG.println(" milliseconds");
  /*
    Serial1.print("got all data in ");
    Serial1.print (StopTime - StartTime);
    Serial1.println(" milliseconds");
  */
}

//*****************************************
void CheckFirstByte() {
  byte x = 0;
  x = recvOneChar();
  if (x == SuccessfulMarker) {
    bytesRecvd = 0;
    inProgress = true;
    allReceived = false;
    // blinkLED(2);
    DEBUG.println("STX received!");
    //Serial1.println("STX received!");
  }
  else if (x == UnuccessfulMarker) {
    bytesRecvd = 0;
    inProgress = false;
    allReceived = false;
    // blinkLED(2);
    DEBUG.println("ETX received!!!!!!");
    //Serial1.println("ETX received!!!!!!");
  }
  else
  {
    DEBUG.print("Received->");
    DEBUG.println(x, DEC);
    //Serial1.print("Received->");
    //Serial1.println(x, HEX);
  }
}
//*****************************************
bool CheckACK() {
  byte x = 0;
  x = recvOneChar();
  if (x == ACK) {
    DEBUG.println("ACK");
    return true;
  }
  else if (x == NAK) {
    DEBUG.println("NAK");
    return false;
  }
  else
  {
    DEBUG.print("Received->");
    DEBUG.println(x, DEC);
    return false;
  }
}
//*****************************************
void GetScan() {
  byte x = 0;

  DEBUG.println("Going binary");
  //Serial1.println("Going binary");
  USB2000.print("bB");
  delay(500);
  while (USB2000.available() > 0)
  {
    x = USB2000.read();
  }
  delay(100);
  DEBUG.println("Getting scan");
  //Serial1.println("Getting scan");
  USB2000.print("S");

}
//*****************************************
uint8_t  recvOneChar() {
  uint32_t StartTime = millis();
  bool CharReceived = false;
  uint8_t receivedChar = 0;

  while (millis() - StartTime <= WaitMillisUSB2000 && CharReceived == false) {
    if (USB2000.available() > 0) {
      receivedChar = USB2000.read();
      //response = DEBUG.read();
      CharReceived = true;
    }
  }
  if (millis() - StartTime >= WaitMillisUSB2000) {
    receivedChar = 99;
  }
  DEBUG.println(receivedChar, DEC);  //debug
  return receivedChar;
}
//*****************************************
void  PrintBuffer() {
  DEBUG.write(tempBuffer, numCharsToReceive);
}
//*****************************************
bool GetAscan() {
  uint8_t response = 99;

  GetScan();                                    //Send scan S command
  CheckFirstByte();                             //Check if first byte is 2=STX -> successful
  if (inProgress == true && allReceived == false)  //If yes get other data
  {
    getSerialData();
    PrintBuffer();
    DEBUG.print("\nMax->");
    DEBUG.println(spectraMax);
    DEBUG.print("Min->");
    DEBUG.println(spectraMin);
    DEBUG.println("\nDone !!!!!");
    //Serial1.println("\nDone !!!!!");
  }
  else      //If not print no data
  {
    DEBUG.println("No data!!!!!");
    //Serial1.println("No data!!!!!");
  }
}
//*****************************************
/*
  void  PrintToPC(String stringa) {
  DEBUG.println(stringa);
  Serial1.println(stringa);
  }
*/
//*****************************************
uint16_t readWord(void) {
  byte x;
  bool WordReceived = false;
  bool firstByte = true;
  uint8_t ulowByte = 0;
  uint8_t uhighByte = 0;
  uint16_t wordRead = 0;
  uint32_t StartTime = millis();

  while (millis() - StartTime <= WaitMillisUSB2000 && WordReceived == false) {
    if (USB2000.available() > 0) {
      x = USB2000.read();
      if (firstByte == true) {
        uhighByte  = x;
        firstByte = false;
        DEBUG.print(uhighByte);
      }
      else
      {
        ulowByte = x;
        wordRead = ulowByte | uhighByte << 8;

        DEBUG.print(" ");
        DEBUG.print(ulowByte);
        DEBUG.print(" ");
        DEBUG.println(wordRead);
        //readWord = wordRead;
        WordReceived = true;
      }
    }

  }
  if (millis() - StartTime >= WaitMillisUSB2000) {
    wordRead = 0;
  }
  return wordRead;
}

