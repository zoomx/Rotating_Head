/*
  Teensy USB2000 communications
*/

const uint8_t SuccessfulMarker = 2;   //STX
const uint8_t UnuccessfulMarker = 3;   //ETX
const uint8_t ACK = 6;
const uint8_t NAK = 21;
const uint32_t WaitMillisUSB2000 = 1000;

uint16_t bytesRecvd = 0;
uint16_t SpecrometerModel;
uint16_t MaxValueFromADC = 65535;
uint16_t BestMaxValue;
uint16_t BestMinValue;

bool CheckEsposition = false;  ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool inProgress = false;
bool allReceived = false;
//*****************************************
bool SpectrometerIsAlive() {
  bool IsAlive = false;
  USB2000.print("v");
  delay(25);
  if (CheckACK() == true) {
    DEBUG.print("Spectrometer version ");
    DEBUG.println(readWord());
    IsAlive = true;
  }
  return IsAlive;
}
//*****************************************
uint16_t SpectrometerVersion() {
  //1040 USB200+
  //FC03 ADC1000-USB
  /*
    Description: Returns the version number of the code running on the microcontroller. A returned
    value of 1000 is interpreted as 1.00.0.

    Identify ADC1000-USB
    Description: Device drivers can differentiate between an ADC1000-USB A/D Converter, a
    USB2000 Spectrometer and a SAD500 Serial Port Interface A/D Converter with the use of this
    command. The ADC1000-USB generates an ACK in response to this command while the other
    instruments generate a NAK.
    Command Syntax: -
    Response: ACK
  */
  uint16_t SpectrometerType = 0;
  bool KnownValue = false;
  USB2000.print("v");
  if (CheckACK() == true) {
    DEBUG.print("Spectrometer version ");
    SpectrometerType = readWord();
    DEBUG.println(SpectrometerType);
    if (SpectrometerType == 0x1040) {
      MaxValueFromADC = 65535;
      KnownValue = true;
    }
    if (SpectrometerType == 0xFC03) {
      MaxValueFromADC = 4095;
      KnownValue = true;
    }
    if (KnownValue == false) {
      MaxValueFromADC = 65535;
      USB2000.print("-");
      if (CheckACK() == true) {
        MaxValueFromADC = 4095;
      }
    }
  }
  else {
    DEBUG.print("Spectrometer not found!!!");
  }
  //BestMaxValue=; //new values here!
  //BestMinValue=;
  return SpectrometerType;
}
//*****************************************
bool CheckExposition() {

  bool response = false;
  bool OkExposition = true;

  response = GetScanFromSpectrometer();

  if (response == false) {
    DEBUG.println("CheckExposition - Can't get spectrum - ERROR!!!!");
    return false;
  }

  DEBUG.print("Starting IntegrationTime->");
  DEBUG.println(IntegrationTime);
  while (OkExposition == false) {
    if (spectraMax  >= HighOptimalLevel) {
      IntegrationTime = IntegrationTime * 0.95;
      OkExposition = false;
      DEBUG.println(IntegrationTime);
    }
    if (spectraMin  >= LowOptimalLevel) {
      IntegrationTime = IntegrationTime * 1.05;
      OkExposition = false;
      DEBUG.println(IntegrationTime);
    }

    if (IntegrationTime >= MaxIntegrationTime ) {
      DEBUG.println("CheckExposition - MaxIntegrationTime!");
      return false;
    }

    response = GetScanFromSpectrometer();
    if (response == false) {
      DEBUG.println("CheckExposition - Cant get spectrum - ERROR!!!!");
      return false;
    }
  }
  return true;
}
//*****************************************
bool GetScanFromSpectrometer() {
  // Send v command to get spectrometer version
  // then go to GetAscan that will send the s command
  // at the end, without errors, the scan is in the buffer

  bool response;

  USB2000.print("v");
  DEBUG.println("v");
  delay(25);
  if (CheckACK() == true) {
    DEBUG.print("Spectrometer version ");
    DEBUG.println(readWord());
  }

  //Here you can check the spectrometer type
  // and change something if necessary

  spectraMax = 0;
  spectraMin = 65535;
  response = GetAscan();
  return response;
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
  char charToCheck;

  spectraMax = 0;
  spectraMin = 65535;

  //Drop first two bytes FF FF
  charToCheck = recvOneChar();
  if (charToCheck == 0xFF) {
    DEBUG.println("First block byte is FF");
  }
  else {
    DEBUG.print("ERROR: First block byte is ");
    DEBUG.println(charToCheck);
  }
  charToCheck = recvOneChar();
  if (charToCheck == 0xFF) {
    DEBUG.println("Second block byte is FF");
  }
  else {
    DEBUG.print("ERROR: Second block byte is ");
    DEBUG.println(charToCheck);
  }

  while (millis() - StartTime <= WaitMillisUSB2000 && allReceived == false) {  //while we have not received all chars
    if (USB2000.available() > 0)      //Are there new data?
    {
      x = USB2000.read();  //Yes!! Read data

      if (inProgress) { // maybe this can be removed
        tempBuffer[bytesRecvd] = x;
        bytesRecvd ++;
      }

      if (bytesRecvd > 16) {  //Header is 16 bytes, last byte is an end packet
        if (firstByte == true) {
          uhighByte  = x;
          firstByte = false;
          //DEBUG.print(uhighByte);
        }
        else {
          ulowByte = x;
          RawValue = ulowByte | uhighByte << 8;

          if (bytesRecvd < numCharsToReceive + 1) {

            if (RawValue > spectraMax) {
              spectraMax = RawValue;
            }

            if (RawValue < spectraMin) {
              spectraMin = RawValue;
            }
            /*
              DEBUG.print(" ");
              DEBUG.print(ulowByte);
              DEBUG.print(" ");

              DEBUG.println(RawValue);
             */
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++
          }

          if (bytesRecvd > numCharsToReceive + 1) {
            DEBUG.println("");
            DEBUG.println(RawValue);
          }

          firstByte = true;
        }
      }

      if (bytesRecvd == numCharsToReceive) {    // Are all data received? add here check timeout
        inProgress = false;   //Yes, stop receive data
        allReceived = true;   //All data received
        //StopTime = millis();
      }
    }
  }


  //Drop last 2 FF FD
  charToCheck = recvOneChar();
  if (charToCheck == 0xFF) {
    DEBUG.println("second-last byte is FF");
  }
  else {
    DEBUG.print("ERROR: second-last byte is ");
    DEBUG.println(charToCheck);
  }
  charToCheck = recvOneChar();
  if (charToCheck == 0xFD) {
    DEBUG.println("Last byte is FD");
  }
  else {
    DEBUG.print("ERROR: Last block byte is ");
    DEBUG.println(charToCheck);
  }
  StopTime = millis();
  DEBUG.print("got all data in ");
  DEBUG.print (StopTime - StartTime);
  DEBUG.println(" milliseconds");
}

//*****************************************
void CheckFirstByte() {
  //check the first byte after an s command
  //It should be STX that mean that command is OK
  //then data will follow
  //If ETX is received it mean an error and no data will follow

  byte x = 0;
  x = recvOneChar();
  if (x == SuccessfulMarker) {
    bytesRecvd = 0;
    inProgress = true;
    allReceived = false;
    // blinkLED(2);
    DEBUG.println("STX received!");
  }
  else if (x == UnuccessfulMarker) {
    bytesRecvd = 0;
    inProgress = false;
    allReceived = false;
    // blinkLED(2);
    DEBUG.println("ETX received!!!!!!");
  }
  else
  {
    DEBUG.print("Received->");
    DEBUG.println(x, DEC);
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
  // Got to binary mode then send S command
  byte x = 0;

  DEBUG.println("Going binary");
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
  //Get a spectrum scan from spectrometer
  //First go to binary mode and send the S command using GetScan();
  //Then check the first byte received is STX and not ETX CheckFirstByte();
  //Then get the remaining data getSerialData();
  uint8_t response = 99;

  GetScan();    //Go to binary mode and send scan S command
  CheckFirstByte();    //Check if first byte is 2=STX -> successful and not ETX=unsuccessful
  if (inProgress == true && allReceived == false)  //If yes (STX received and not ETX) get other data
  {
    //loop
    getSerialData();
    //PrintBuffer();
    DEBUG.print("\nMax->");
    DEBUG.println(spectraMax);
    DEBUG.print("Min->");
    DEBUG.println(spectraMin);
    DEBUG.println("\nDone !!!!!");
    //Serial1.println("\nDone !!!!!");

    //check exposition
    //if CheckEsposition = false; -> exposition is good!
    //until exposition is good
    return true;
  }
  else      //If not (ETX received or other else than STX or timeout) print no data
  {
    DEBUG.println("No data!!!!!");
    return false;
  }
}
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
        WordReceived = true;
      }
    }
  }

  if (millis() - StartTime >= WaitMillisUSB2000) {
    wordRead = 0;
  }
  return wordRead;
}

