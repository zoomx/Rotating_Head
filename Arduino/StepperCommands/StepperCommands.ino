#define VERSION "1.0"
#define SUBVERSION "3"

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

  20180724
  Commands on SoftSerial
  Debug on Hardware Serial
  GOTO doesn't work!!!

  20180725
  GOTO fixed example GOTO 2456/r/n
  #define MAXSERIALCOMMANDS 12 Otherwise you can add only 10 commands
  STEPS added

  2018 07 27
  recvWithEndMarker removed

    wiring
  ArduinoPin    L298nPin  MotorWire Signal
  3
  8             In1         White     A+
  9             In2         Brown     A-
  //10            //In3         //Blue      //B+
  11            In4         Black     B-
  12
  13


  4 SoftwareSerial RX
  5 SoftwareSerial TX
*/

#include <Stepper.h>
#include "SerialCommand.h"
#include <SoftwareSerial.h>

#define DEBUG Serial
#define BAUDSSERIAL 9600
#define BAUDDEBUG 115200

SoftwareSerial SSerial(4, 5); // RX, TX
SerialCommand sCmd(SSerial);
//SerialCommand sCmd;

// these are used for finding zero, it can be a pin or a hole.
#define NOZERO 0
#define ZERO 1
bool NoZero_b = 0;
bool Zero_b = 1;


const uint16_t stepsPerRevolution = 3000;  //Should be 12000
const byte ZeroPin = 16; //A2 IN2


const byte pwmA = 3;
const byte pwmB = 11;
const byte brakeA = 9;
const byte brakeB = 8;
const byte dirA = 12;
const byte dirB = 13;

const uint16_t TurnPasses = 12000;
const uint16_t QuarterPasses = TurnPasses / 4;
const uint16_t MaxSearchPasses = TurnPasses + QuarterPasses;
const int16_t HalfMaxPass = TurnPasses / 2;

//positions
const uint16_t HeadPark = 0;
const uint8_t NumScans = 50;
uint16_t HeadStart = HeadPark + QuarterPasses;
uint16_t HeadZenith = HeadPark + QuarterPasses * 2;
uint16_t HeadStop = HeadPark + QuarterPasses * 3;
uint16_t MeasSteps = (HeadStop - HeadStart) / NumScans;

uint16_t ActualPosition = 0;

//degrees Head Start and Stop
const uint16_t HeadStartDegrees = 0;
const uint16_t HeadStopDegrees = 0;

int16_t steps = 0;  //steps can be negative!


const uint32_t WaitMillis = 10000;

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 12, 13);

uint32_t TimeStart = 0;




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
    SSerial.println(F("ERR"));
    PrintElapsedTime();
    return;
  }

  DEBUG.print(steps);
  DEBUG.println(F("Found zero!"));
  PrintElapsedTime();
  blink();
  ActualPosition = 0;
  SSerial.println(F("OK"));
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
    SSerial.println(F("ERR"));
    PrintElapsedTime();
    return;
    }
    ActualPosition = 0;

    DEBUG.println(F(" found zero second time"));
    DEBUG.print(steps);
    DEBUG.println(F(" total steps"));
    PrintElapsedTime();
    SSerial.println(F("OK"));
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
    SSerial.println(F("ERR"));
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
    SSerial.println(F("ERR"));
    PrintElapsedTime();
    return;
  }
  ActualPosition = 0;

  DEBUG.println(F(" found zero second time"));
  DEBUG.print(steps);
  DEBUG.println(F(" total steps"));
  PrintElapsedTime();
  DEBUG.println(steps);
  SSerial.println(F("OK"));

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
  SSerial.println(F("OK"));
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
  DEBUG.println(F("PARK"));
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadPark); //HeadPark - ActualPosition;
  DEBUG.print(F("Diff to Park "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Start() {
  DEBUG.println(F("START"));
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStart); // HeadStart - ActualPosition;
  DEBUG.print(F("Diff to Start "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Next() {
  DEBUG.println(F("NEXT"));
  TimeStart = millis();
  myStepper.step(MeasSteps);
  ActualPosition = ActualPosition + MeasSteps;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Stop() {
  DEBUG.println(F("STOP"));
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStop);// HeadStop - ActualPosition;
  DEBUG.print(F("Diff to Stop "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Goto() {
  char *arg;
  DEBUG.println(F("GOTO"));
  TimeStart = millis();
  //recvWithEndMarker();

  arg = sCmd.next();
  DEBUG.println(arg);

  steps = atoi(arg);
  DEBUG.println(steps);
  /*
    String response = arg;
    DEBUG.println(response);
    steps = response.toInt();
    DEBUG.println(steps);
  */
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadZenith); // HeadZenith - ActualPosition;
  DEBUG.print("Diff to New Position ");
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void MakeSteps() {
  char *arg;
  DEBUG.println(F("STEPS"));
  TimeStart = millis();
  //recvWithEndMarker();

  arg = sCmd.next();
  DEBUG.println(arg);

  steps = atoi(arg);
  DEBUG.println(steps);
  /*
    String response = arg;
    DEBUG.println(response);
    steps = response.toInt();
    DEBUG.println(steps);
  */

  myStepper.step(steps);
  ActualPosition = ActualPosition + steps;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Zenith() {
  DEBUG.println(F("ZENITH"));
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadZenith); // HeadZenith - ActualPosition;
  DEBUG.print("Diff to Zenith ");
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  PrintElapsedTime();
  SSerial.println(F("OK"));
}
//*********************************************************
void Hello() {
  DEBUG.println(F("HELLO"));
  SSerial.println(F("OK"));
}
//*********************************************************
void GetPos() {
  DEBUG.print(F("ActualPosition "));
  SSerial.println(ActualPosition);
  DEBUG.println(ActualPosition);
}
//*********************************************************
void PrintVars() {
  SSerial.println(F("Print vars"));
  SSerial.print("stepsPerRevolution ");
  SSerial.println(stepsPerRevolution);
  SSerial.print(F("ZeroPin "));
  //SSerial.println(ZeroPin);
  //SSerial.print(F("ZeroPin "));
  SSerial.println(ZeroPin);
  SSerial.print(F("TurnPasses "));
  SSerial.println(TurnPasses);
  SSerial.print(F("QuarterPasses "));
  SSerial.println(QuarterPasses);
  SSerial.print(F("MaxSearchPasses "));
  SSerial.println(MaxSearchPasses);
  SSerial.print(F("HeadPark "));
  SSerial.println(HeadPark);
  SSerial.print(F("HeadStart "));
  SSerial.println(HeadStart);
  SSerial.print(F("HeadZenith "));
  SSerial.println(HeadZenith);
  SSerial.print(F("HeadStop "));
  SSerial.println(HeadStop);
  SSerial.print(F("MeasSteps "));
  SSerial.println(MeasSteps);
  SSerial.print(F("ActualPosition "));
  SSerial.println(ActualPosition);
  SSerial.print(F("HeadStartDegrees "));
  SSerial.println(HeadStartDegrees);
  SSerial.print(F("HeadStopDegrees "));
  SSerial.println(HeadStopDegrees);
  SSerial.print(F("WaitMillis "));
  SSerial.println(WaitMillis);
  SSerial.print(F("Version "));
  SSerial.println(VERSION);
  DEBUG.println(VERSION);
  SSerial.print(F("SubVersion "));
  SSerial.println(SUBVERSION);
  DEBUG.println(SUBVERSION);
  SSerial.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  DEBUG.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  /*
    SSerial.print("pwmA ");
    SSerial.println(pwmA);
    SSerial.print("pwmB ");
    SSerial.println(pwmB);


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
  SSerial.println(F("CMDERR"));
  DEBUG.println(F("CMDERR"));
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

  SSerial.begin(BAUDSSERIAL);
  DEBUG.begin(BAUDDEBUG);
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
  sCmd.addCommand("ZENITH", Zenith);
  sCmd.addCommand("GETPOS", GetPos);
  sCmd.addCommand("PRINTVARS", PrintVars);
  sCmd.addCommand("ZTYPE", SlitOrPin);
  sCmd.addCommand("HELLO", Hello);
  sCmd.addCommand("GOTO", Goto);
  sCmd.addCommand("STEPS", MakeSteps);
  //sCmd.addCommand("DUMMY", Hello); //it seems that the last add is never recognized!
  //sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?") Old Library
  sCmd.addDefaultHandler(unrecognized);       //New Library
  DEBUG.println(F("Waiting for commands"));
}
//*********************************************************
//*********************************************************
void loop() {
  sCmd.readSerial();

}



