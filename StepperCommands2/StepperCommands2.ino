#define VERSION "1.0"
#define SUBVERSION "0"

/*
  StepperCommands2
  using Arduino Motor Shield rev3
  https://store-usa.arduino.cc/products/arduino-motor-shield-rev3

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

  2021 04 14
  Now Command serial is the main serial
  Debug serial is software serial
  Added command for counting steps per revolution

  2021 12 13
  Small corrections

  2021 12 23
  Added Command NEXTI that doesn't turn off motor
  GOTO corrected

  2022 01 11
  Init now go only to zero

  2022 02 08
  Some changes in PRINTVARS
  Changed NoZero_b in Zero_b in INIT and CSTEPS

  2022 03 03
  SerialCommand library changed
  https://github.com/ppedro74/Arduino-SerialCommands
  
    wiring
  ArduinoPin    L298nPin  MotorWire Signal
  3
  4             SoftwareSerial RX  9600
  5             SoftwareSerial TX  also OUT5
  6             OUT6
  7
  8             In1         White     A+
  9             In2         Brown     A-
  10            In3         Blue      B+
  11            In4         Black     B-
  12
  13
  14  A0        Current Sensing ChA
  15  A1        Current Sensing ChB
  16  A2        Zero pin
  17  A3        Input pin free
  18  A4
  19  A5


    Screw terminal to connect the motors and their power supply.
    2 TinkerKit connectors for two Analog Inputs (in white), connected to A2 and A3.
    2 TinkerKit connectors for two Analog Outputs (in orange in the middle),
      connected to PWM outputs on pins D5 and D6.
    2 TinkerKit connectors for the TWI interface (in blue with 4 pins),
      one for input and the other one for output.

*/

#include <Stepper.h>
#include <SerialCommands.h>
#include <SoftwareSerial.h>

#define SERIAL Serial
#define DEBUG SSerial
#define BAUDSSERIAL 115200
#define BAUDDEBUG 9600

SoftwareSerial SSerial(4, 5); // RX, TX
//SerialCommand sCmd(SSerial);
//SerialCommand sCmd;
char sCmd_buffer_[10];
SerialCommands sCmd(&SERIAL, sCmd_buffer_, sizeof(sCmd_buffer_), "\r\n", " ");


// these are used for finding zero, it can be a pin or a hole.
#define NOZERO 0
#define ZERO 1
bool NoZero_b = 0;
bool Zero_b = 1;


const uint16_t stepsPerRevolution = 1500;
//This is not the real value
//Used for stepper velocity
//12000 is too much for RS motor

const byte ZeroPin = 16; //A2 IN2


const byte pwmA = 3;
const byte pwmB = 11;
const byte brakeA = 9;
const byte brakeB = 8;
const byte dirA = 12;
const byte dirB = 13;

const uint16_t TurnPasses = 3000;
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
void Init(SerialCommands* sender) {
  DEBUG.println(F("INIT"));
  ActualPosition = 15000;     //That is no position known
  steps = 0;
  boolean found = false;

  DEBUG.println(F("Searching for zero"));

  ResumeMotorPins();

  do
  {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == Zero_b)  //0 if there is a fissure, 1 if there is a pin.
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  DEBUG.println(F("End of search"));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found after more than a revolution"));
    SERIAL.println(F("ERR"));
    PrintElapsedTime();

  }
  else {
    DEBUG.print(steps);
    DEBUG.println(F("Found zero!"));
    PrintElapsedTime();
    blink();
    ActualPosition = 0;
    SERIAL.println(F("OK"));
  }

  LowAllMotorPins();
  return;


  /*

    // step one revolution  in one direction:
    DEBUG.println(F("positive direction"));
    ResumeMotorPins();
    TimeStart = millis();
    DEBUG.println(F("Searching pin or slit"));
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
      SERIAL.println(F("ERR"));
      PrintElapsedTime();
      return;
    }

    DEBUG.print(steps);
    DEBUG.println(F("Found zero!"));
    PrintElapsedTime();
    blink();
    ActualPosition = 0;
    SERIAL.println(F("OK"));
    return;
  */

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
    SERIAL.println(F("ERR"));
    PrintElapsedTime();
    return;
    }
    ActualPosition = 0;

    DEBUG.println(F(" found zero second time"));
    DEBUG.print(steps);
    DEBUG.println(F(" total steps"));
    PrintElapsedTime();
    SERIAL.println(F("OK"));
  */
}
//*********************************************************
void FindStepsNumber(SerialCommands* sender) {
  DEBUG.println(F("FindStepsNumber"));
  ActualPosition = 15000;     //That is no position known
  steps = 0;
  boolean found = false;
  // step one revolution  in one direction:
  DEBUG.println(F("positive direction"));

  ResumeMotorPins();

  TimeStart = millis();

  do
  {
    myStepper.step(1);
    steps += 1;
    if (digitalRead(ZeroPin) == Zero_b)  //0 if there is a fissure, 1 if there is a pin.
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found first time after more than a revolution"));
    SERIAL.println(F("ERR"));
    PrintElapsedTime();
    LowAllMotorPins();
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
    if (digitalRead(ZeroPin) == Zero_b)
    {
      found = true;
    }
  } while ((found == false) && (steps < MaxSearchPasses));

  if (steps >= MaxSearchPasses) {
    DEBUG.println(F("Zero not found second time after more than a revolution"));
    SERIAL.println(F("ERR"));
    PrintElapsedTime();
    LowAllMotorPins();
    return;
  }
  ActualPosition = 0;

  DEBUG.println(F(" found zero second time"));
  DEBUG.print(steps);
  DEBUG.println(F(" total steps"));
  PrintElapsedTime();
  DEBUG.println(steps);
  //SERIAL.println(F("OK"));
  SERIAL.println(steps);
  LowAllMotorPins();
}
//*********************************************************
bool SlitOrPin(SerialCommands* sender)
{
  DEBUG.println(F("Slit or Pin"));
  DEBUG.println(F("Start search"));

  uint16_t Uno = 0;
  uint16_t Zero = 0;
  ResumeMotorPins();
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
      DEBUG.println("Pin detected");
      NoZero_b = 1;
      Zero_b = 0;
    }
    else {
      DEBUG.println("Slit detected");
      NoZero_b = 0;
      Zero_b = 1;
    }

  }
  steps = 0;
  if (Uno == 0 || Zero == 0) {
    steps = -1000;
  }
  myStepper.step(steps);
  SERIAL.println(F("OK"));
  LowAllMotorPins();
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
void Park(SerialCommands* sender) {
  DEBUG.println(F("PARK"));
  ResumeMotorPins();
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadPark); //HeadPark - ActualPosition;
  DEBUG.print(F("Diff to Park "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Start(SerialCommands* sender) {
  DEBUG.println(F("START"));
  ResumeMotorPins();
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStart); // HeadStart - ActualPosition;
  DEBUG.print(F("Diff to Start "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Next(SerialCommands* sender) {
  DEBUG.println(F("NEXT"));
  ResumeMotorPins();
  TimeStart = millis();
  myStepper.step(MeasSteps);
  ActualPosition = ActualPosition + MeasSteps;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Nexti(SerialCommands* sender) {
  DEBUG.println(F("NEXTI"));
  ResumeMotorPins();
  TimeStart = millis();
  myStepper.step(MeasSteps);
  ActualPosition = ActualPosition + MeasSteps;
  //LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Stop(SerialCommands* sender) {
  DEBUG.println(F("STOP"));
  ResumeMotorPins();
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadStop);// HeadStop - ActualPosition;
  DEBUG.print(F("Diff to Stop "));
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Goto(SerialCommands* sender) {
  char *arg;
  DEBUG.println(F("GOTO"));
  ResumeMotorPins();
  TimeStart = millis();
  //recvWithEndMarker();

  arg = sender->Next(); //sCmd.next();
  DEBUG.println(arg);

  steps = atoi(arg);
  DEBUG.println(steps);

  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, steps);
  DEBUG.print("Diff to New Position ");
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
  //SERIAL.println(F("OK"));
}
//*********************************************************
void MakeSteps(SerialCommands* sender) {
  char *arg;
  DEBUG.println(F("STEPS"));
  ResumeMotorPins();
  TimeStart = millis();
  //recvWithEndMarker();

  arg = sender->Next(); //sCmd.next();
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
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Zenith(SerialCommands* sender) {
  DEBUG.println(F("ZENITH"));
  ResumeMotorPins();
  TimeStart = millis();
  int16_t diff = 0;
  diff = calculateDifferenceBetweenSteps(ActualPosition, HeadZenith); // HeadZenith - ActualPosition;
  DEBUG.print("Diff to Zenith ");
  DEBUG.println(diff);
  myStepper.step(diff);
  ActualPosition = ActualPosition + diff;
  LowAllMotorPins();
  PrintElapsedTime();
  SERIAL.println(F("OK"));
}
//*********************************************************
void Hello(SerialCommands* sender) {
  DEBUG.println(F("HELLO"));
  SERIAL.println(F("OK"));
}
//*********************************************************
void GetPos(SerialCommands* sender) {
  DEBUG.print(F("ActualPosition "));
  SERIAL.println(ActualPosition);
  DEBUG.println(ActualPosition);
}
//*********************************************************
void PrintVars(SerialCommands* sender) {
  SERIAL.println(F("Print vars"));
  SERIAL.print("stepsPerRevolution ");
  SERIAL.println(stepsPerRevolution);
  SERIAL.print(F("ZeroPin "));
  SERIAL.println(ZeroPin);
  SERIAL.print(F("ZeroPin Value->"));
  SERIAL.println(digitalRead(ZeroPin));
  SERIAL.print(F("NoZero_b Value->"));
  SERIAL.println(NoZero_b);
  SERIAL.print(F(" "));
  SERIAL.println(digitalRead(ZeroPin));
  SERIAL.print(F("TurnPasses "));
  SERIAL.println(TurnPasses);
  SERIAL.print(F("QuarterPasses "));
  SERIAL.println(QuarterPasses);
  SERIAL.print(F("MaxSearchPasses "));
  SERIAL.println(MaxSearchPasses);
  SERIAL.print(F("HeadPark "));
  SERIAL.println(HeadPark);
  SERIAL.print(F("HeadStart "));
  SERIAL.println(HeadStart);
  SERIAL.print(F("HeadZenith "));
  SERIAL.println(HeadZenith);
  SERIAL.print(F("HeadStop "));
  SERIAL.println(HeadStop);
  SERIAL.print(F("MeasSteps "));
  SERIAL.println(MeasSteps);
  SERIAL.print(F("ActualPosition "));
  SERIAL.println(ActualPosition);
  SERIAL.print(F("HeadStartDegrees "));
  SERIAL.println(HeadStartDegrees);
  SERIAL.print(F("HeadStopDegrees "));
  SERIAL.println(HeadStopDegrees);
  SERIAL.print(F("WaitMillis "));
  SERIAL.println(WaitMillis);
  SERIAL.print(F("Version "));
  SERIAL.println(VERSION);
  DEBUG.println(VERSION);
  SERIAL.print(F("SubVersion "));
  SERIAL.println(SUBVERSION);
  DEBUG.println(SUBVERSION);
  SERIAL.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  DEBUG.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  /*
    SERIAL.print("pwmA ");
    SERIAL.println(pwmA);
    SERIAL.print("pwmB ");
    SERIAL.println(pwmB);


    const int pwmA = 3;
    const int pwmB = 11;
    const int brakeA = 9;
    const int brakeB = 8;
    const int dirA = 12;
    const int dirB = 13;
  */
}

//*********************************************************
void unrecognized(SerialCommands* sender, const char* cmd) {
  SERIAL.println(F("CMDERR"));
  DEBUG.println(F("CMDERR"));
  DEBUG.println(cmd);
}
//*********************************************************
void LowAllMotorPins() {
  digitalWrite(pwmA, LOW);
  digitalWrite(pwmB, LOW);
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);
}
//*********************************************************
void ResumeMotorPins() {
  digitalWrite(pwmA, HIGH);
  digitalWrite(pwmB, HIGH);
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);
}
//*********************************************************
//Note: Commands are case sensitive
SerialCommand Init_("INIT", Init);
SerialCommand Park_("PARK", Park);
SerialCommand Start_("START", Start);
SerialCommand Next_("NEXT", Next);
SerialCommand Stop_("STOP", Stop);
SerialCommand Zenith_("ZENITH", Zenith);
SerialCommand GetPos_("GETPOS", GetPos);
SerialCommand PrintVars_("PRINTVARS", PrintVars);
SerialCommand SlitOrPin_("ZTYPE", SlitOrPin);
SerialCommand Hello_("HELLO", Hello);
SerialCommand Goto_("GOTO", Goto);
SerialCommand MakeSteps_("STEPS", MakeSteps);
SerialCommand FindStepsNumber_("CSTEPS", FindStepsNumber);
SerialCommand Nexti_("NEXTI", Nexti);

//*********************************************************
//*********************************************************
void setup() {

  myStepper.setSpeed(3);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);
  digitalWrite(pwmA, LOW); //was high
  digitalWrite(pwmB, LOW); //was high
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);
  LowAllMotorPins();

  pinMode(ZeroPin, INPUT);   //maybe use INPUT_PULLUP?

  SERIAL.begin(BAUDSSERIAL);
  //SSerial.begin(BAUDSSERIAL);
  DEBUG.begin(BAUDDEBUG);
  DEBUG.println(F("StepperCommands"));

  for (int i = 0; i <= 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }

  sCmd.AddCommand(&Init_); //("INIT", Init);  //1
  sCmd.AddCommand(&Park_); //("PARK", Park);  //2
  sCmd.AddCommand(&Start_); //("START", Start);  //3
  sCmd.AddCommand(&Next_); //("NEXT", Next);  //4
  sCmd.AddCommand(&Stop_); //("STOP", Stop);  //5
  sCmd.AddCommand(&Zenith_); //("ZENITH", Zenith);  //6
  sCmd.AddCommand(&GetPos_); //("GETPOS", GetPos);  //7
  sCmd.AddCommand(&PrintVars_); //("PRINTVARS", PrintVars);  //8
  sCmd.AddCommand(&SlitOrPin_); //("ZTYPE", SlitOrPin);  //9
  sCmd.AddCommand(&Hello_); //("HELLO", Hello);  //10
  sCmd.AddCommand(&Goto_); //("GOTO", Goto);  //11
  sCmd.AddCommand(&MakeSteps_); //("STEPS", MakeSteps);  //12
  sCmd.AddCommand(&FindStepsNumber_); //("CSTEPS", FindStepsNumber); //13
  sCmd.AddCommand(&Nexti_); //("NEXTI", Nexti);  //14
  //sCmd.addCommand("DUMMY", Hello); //it seems that the last add is never recognized!
  //sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?") Old Library
  sCmd.SetDefaultHandler(unrecognized);       //New Library

  DEBUG.print(F("TurnPasses "));
  DEBUG.println(TurnPasses);
  DEBUG.print(F("MeasSteps "));
  DEBUG.println(MeasSteps);
  DEBUG.println(F("Waiting for commands"));
}
//*********************************************************
//*********************************************************
void loop() {
  sCmd.ReadSerial();

}
