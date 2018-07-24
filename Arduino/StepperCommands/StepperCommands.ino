#define VERSION "1.0"

/*
  StepperCommands
  was  Stepper Motor Control - one revolution
  check for zero signal.
  Find zero twice and count the steps

  by zoomx 20 Dec 2016

  20180710
  Added serial commands and a lot of routines

  20180718
  routine fixed

    wiring
  ArduinoPin    L298nPin  MotorWire Signal
  8             In1         White     A+
  9             In2         Brown     A-
  10            In3         Blue      B+
  11            In4         Black     B-
*/

#include <Stepper.h>
#include <SerialCommand.h>
#include <SoftwareSerial.h>

#define DEBUG debug

SoftwareSerial debug(4, 5); // RX, TX

// these are used for finding zero, it can be a pin or a hole.
#define NOZERO 0
#define ZERO 1
bool NoZero_b = 0;
bool Zero_b = 1;


const int stepsPerRevolution = 3000;  //Should be 12000
const byte ZeroPin = 16; //A2 IN2


const int pwmA = 3;
const int pwmB = 11;
const int brakeA = 9;
const int brakeB = 8;
const int dirA = 12;
const int dirB = 13;

const uint16_t TurnPasses = 12000;
const uint16_t QuarterPasses = TurnPasses / 4;
const uint16_t MaxSearchPasses = TurnPasses + QuarterPasses;
const int16_t HalfMaxPass = TurnPasses / 2;

//positions
const uint16_t HeadPark = 0;
uint16_t HeadStart = HeadPark + QuarterPasses;
uint16_t HeadZenith = HeadPark + QuarterPasses * 2;
uint16_t HeadStop = HeadPark + QuarterPasses * 3;
uint16_t MeasSteps = (HeadStop - HeadStart) / 50;

uint16_t ActualPosition = 0;

//degrees Head Start and Stop
const uint16_t HeadStartDegrees = 0;
const uint16_t HeadStopDegrees = 0;

int16_t steps = 0;

//SerialInputNewline
const byte numChars = 10;
char receivedChars[numChars];  // an array to store the received data
boolean newData = false;
const uint32_t WaitMillis = 10000;
byte ndx = 0;
const char endMarker = 13;   //'\r';

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 12, 13);

uint32_t TimeStart = 0;

SerialCommand sCmd;

//*********************************************************
int16_t recvWithEndMarker() {
  ndx = 0;

  char rc;
  uint32_t StartTime = millis();
  newData = false;

  // if (Serial.available() > 0) {
  while (Serial.available() > 0 && newData == false && millis() - StartTime <= WaitMillis ) {
    rc = Serial.read();
    //Serial.println(rc);

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      //ndx = 0;
      newData = true;

    }
  }
  DEBUG.println(receivedChars);
  return atoi(receivedChars);
}


//*********************************************************
void Init() {
  DEBUG.println(F("INIT"));
  ActualPosition = 15000;     //That is no position known
  steps = 0;
  boolean found = false;
  // step one revolution  in one direction:
  DEBUG.println(F("positive direction"));
  TimeStart = millis();

  do
  {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == NoZero_b)  //0 if there is a fissure, 1 if there is a pin.
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found after more than a revolution"));
    Serial.println(F("ERR"));
    PrintElapsedTime();
    return;
  }

  DEBUG.print(steps);
  DEBUG.println(F("Found zero!"));
  PrintElapsedTime();
  blink();
  ActualPosition = 0;
  Serial.println(F("OK"));
  return;
  /*
    TimeStart = millis();
    steps = 1000;
    myStepper.step(1000);
    found = false;
    do
    {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == NoZero_b)
    {
      found = true;
    }
    } while ((found == false) && (steps < MaxSearchPasses));

    if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found second time after more than a revolution"));
    Serial.println(F("ERR"));
    PrintElapsedTime();
    return;
    }
    ActualPosition = 0;

    DEBUG.println(F(" found zero second time"));
    DEBUG.print(steps);
    DEBUG.println(F(" total steps"));
    PrintElapsedTime();
    Serial.println(F("OK"));
  */
}
//*********************************************************
void FindStepsNumber() {
  DEBUG.println(F("FindStepsNumber"));
  ActualPosition = 15000;     //That is no position known
  steps = 0;
  boolean found = false;
  // step one revolution  in one direction:
  DEBUG.println(F("positive direction"));
  TimeStart = millis();

  do
  {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == NoZero_b)  //0 if there is a fissure, 1 if there is a pin.
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found first time after more than a revolution"));
    Serial.println(F("ERR"));
    PrintElapsedTime();
    return;
  }

  DEBUG.print(steps);
  DEBUG.println(F("Found zero!"));
  PrintElapsedTime();
  blink();
  ActualPosition = 0;

  TimeStart = millis();
  steps = 1000;
  myStepper.step(1000); //goes over slit or pin
  found = false;
  do
  {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == NoZero_b)
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found second time after more than a revolution"));
    Serial.println(F("ERR"));
    PrintElapsedTime();
    return;
  }
  ActualPosition = 0;

  DEBUG.println(F(" found zero second time"));
  DEBUG.print(steps);
  DEBUG.println(F(" total steps"));
  PrintElapsedTime();
  DEBUG.println(steps);
  Serial.println(F("OK"));

}
//*********************************************************
bool SlitOrPin()
{
  DEBUG.println(F("Slit or Pin"));
  DEBUG.println(F("Start search"));

  uint16_t Uno = 0;
  uint16_t Zero = 0;
  for (int i = 0; i <= 1000; i++) {
    myStepper.step(1);
    if (digitalRead(ZeroPin) == 0)
    {
      Zero++;
    }
    else
    {
      Uno++;
    }
  }
  DEBUG.print("Zero=");
  DEBUG.println(Zero);
  DEBUG.print("Uno=");
  DEBUG.println(Uno);
  if (Zero == Uno) {
    DEBUG.print("Error, Zero and Uno are equals!");
  }
  else  {
    if (Zero > Uno) {
      DEBUG.print("Pin detected");
      NoZero_b = 1;
      Zero_b = 0;
    }
    else {
      DEBUG.print("Slit detected");
      NoZero_b = 0;
      Zero_b = 1;
    }

  }
  steps = 0;
  if (Uno == 0 || Zero == 0) {
    steps = -1000;
  }
  myStepper.step(steps);
  Serial.println(F("OK"));
}
//*********************************************************
int16_t calculateDifferenceBetweenSteps(int16_t startPosition, int16_t endPosition)
{
  int16_t difference = endPosition - startPosition;
  while (difference < -HalfMaxPass) difference += TurnPasses;
  while (difference > HalfMaxPass) difference -= TurnPasses;
  return difference;
}
//*********************************************************
void PrintElapsedTime() {
  DEBUG.print(F("TIME->"));
  DEBUG.println(millis() - TimeStart);
}
//*********************************************************
void blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}
//*********************************************************
void Park() {
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadPark); //HeadPark - ActualPosition;
  DEBUG.print(F("Diff to Park "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void Start() {
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStart); // HeadStart - ActualPosition;
  DEBUG.print(F("Diff to Start "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void Next() {
  TimeStart = millis();
  myStepper.step(MeasSteps);
  ActualPosition = ActualPosition + MeasSteps;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void Stop() {
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStop);// HeadStop - ActualPosition;
  DEBUG.print(F("Diff to Stop "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void Goto() {
  TimeStart = millis();
  steps = receivedChars;
  myStepper.step(steps);
  ActualPosition = ActualPosition + steps;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void Zenith() {
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadZenith); // HeadZenith - ActualPosition;
  DEBUG.print("Diff to Zenith ");
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  Serial.println(F("OK"));
}
//*********************************************************
void GetPos() {
  DEBUG.print(F("ActualPosition "));
  Serial.println(ActualPosition);
}
//*********************************************************
void PrintVars() {
  Serial.println(F("Print vars"));
  Serial.print("stepsPerRevolution ");
  Serial.println(stepsPerRevolution);
  Serial.print(F("ZeroPin "));
  //Serial.println(ZeroPin);
  //Serial.print(F("ZeroPin "));
  Serial.println(ZeroPin);
  Serial.print(F("TurnPasses "));
  Serial.println(TurnPasses);
  Serial.print(F("QuarterPasses "));
  Serial.println(QuarterPasses);
  Serial.print(F("MaxSearchPasses "));
  Serial.println(MaxSearchPasses);
  Serial.print(F("HeadPark "));
  Serial.println(HeadPark);
  Serial.print(F("HeadStart "));
  Serial.println(HeadStart);
  Serial.print(F("HeadZenith "));
  Serial.println(HeadZenith);
  Serial.print(F("HeadStop "));
  Serial.println(HeadStop);
  Serial.print(F("MeasSteps "));
  Serial.println(MeasSteps);
  Serial.print(F("ActualPosition "));
  Serial.println(ActualPosition);
  Serial.print(F("HeadStartDegrees "));
  Serial.println(HeadStartDegrees);
  Serial.print(F("HeadStopDegrees "));
  Serial.println(HeadStopDegrees);
  Serial.print(F("WaitMillis "));
  Serial.println(WaitMillis);
  Serial.print(F("endMarker "));
  Serial.println(endMarker, HEX);
  Serial.print(F("Version "));
  Serial.println(VERSION);
  /*
    Serial.print("pwmA ");
    Serial.println(pwmA);
    Serial.print("pwmB ");
    Serial.println(pwmB);


    const int pwmA = 3;
    const int pwmB = 11;
    const int brakeA = 9;
    const int brakeB = 8;
    const int dirA = 12;
    const int dirB = 13;
  */
}

//*********************************************************
void unrecognized(const char *command) {
  Serial.println(F("CMDERR"));

}
//*********************************************************
//*********************************************************
void setup() {

  myStepper.setSpeed(3);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);
  digitalWrite(pwmA, HIGH);
  digitalWrite(pwmB, HIGH);
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);

  pinMode(ZeroPin, INPUT);   //maybe use INPUT_PULLUP?

  Serial.begin(9600);
  DEBUG.begin(9600);
  DEBUG.println(F("StepperCommands"));

  for (int i = 0; i <= 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }

  sCmd.addCommand("INIT", Init);
  sCmd.addCommand("PARK", Park);
  sCmd.addCommand("START", Start);
  sCmd.addCommand("NEXT", Next);
  sCmd.addCommand("STOP", Stop);
  sCmd.addCommand("GOTO", Goto);
  sCmd.addCommand("ZENITH", Zenith);
  sCmd.addCommand("GETPOS", GetPos);
  sCmd.addCommand("PRINTVARS", PrintVars);
  sCmd.addCommand("ZTYPE", SlitOrPin);
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")

  DEBUG.println(F("Waiting for commands"));
}
//*********************************************************
//*********************************************************
void loop() {
  sCmd.readSerial();

}



