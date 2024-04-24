//-------------------------------------------------------------------------------------------------------------
//NTF Automatic circuit pressure controller
//Vladislav Petrov
//Sean Rolandelli
//Bradley Sullivan
//Last modification April 18, 2024
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//Global Variables and Libraries
//-------------------------------------------------------------------------------------------------------------
#include <SD.h>                 //Used for SD Card reader
#include <SPI.h>                //Used for serial communication
#include <PID_v1.h>             //used for PID Controller
#include <EasyNextionLibrary.h> //Used for touchscreen display
#include <Trigger.h>            //Used to define trigger functions
#include <AUnit.h>              //Unit test library
#include <AUnitVerbose.h>       //Unit test library
#include "CRC8.h"               //Checksum Library
#include "CRC.h"                //Checksum Library

//Set pins for controlling circuit solenoids, these are global
const int marxoutPin = 22;
const int marxinPin = 23;
const int mtgoutPin = 24;
const int mtginPin = 25;
const int switchoutPin = 26;
const int switchinPin = 27;
const int tg70switchoutPin = 28;
const int tg70switchinPin = 29;
const int tg70marxoutPin = 30;
const int tg70marxinPin = 31;

//Ser pins for controlling reclaimer relays, these are global.
const int reclaimerstopPin = 32;
const int reclaimerstartPin = 33;

//Set pins for reading analog data from pressure sensors, these are global
const int marxanaloginPin = A0;
const int switchanaloginPin = A1;
const int mtganaloginPin = A2;
const int tg70marxanaloginPin = A3;
const int tg70switchanaloginPin = A4;
const int reclaimeranaloginPin = A5;
const int bottleanaloginPin = A6;

//Set pins for control buttons, these are global
const int shotmodePin = 34;
const int purgePin = 35;
const int alarmPin = 36;
const int automatereclaimerPin = 37;
const int abortbuttonPin = 2; // Changed from pin 1 to pin 2

//Set pins for enabling/disabling each system
const int marxenablePin = 17;
const int mtgenablePin = 18;
const int switchenablePin = 19;
const int tg70switchenablePin = 20;
const int tg70marxenablePin = 21;

//Set pins for automatic reclaimer control
const int reclaimerstopenablePin = 4;
const int reclaimerstartenablePin =3;

//Set pin for controlling alarm sound
const int alarmsoundPin = 16;
  
//Set pins for control button LEDs, these are global
const int marxenableLEDPin = 38;
const int mtgenableLEDPin = 39;
const int switchenableLEDPin = 40;
const int tg70switchenableLEDPin = 41;
const int tg70marxenableLEDPin = 42;
const int alarmLEDPin = 43;
const int abortLEDPin = 44;
const int shotmodeLEDPin = 45;
const int purgeLEDPin = 46;

const int startLEDPin = 47;
const int stopLEDPin = 48;
const int automatereclaimerLEDPin = 49;

//Set pins for SD card reader
const int misopin = 50;
const int mosiPin = 51;
const int sckPin = 52;
const int csPin = 53;

//Touchscreen
EasyNex myNex(Serial3);
//GREEN WIRE = RX
//WHITE WIRE = TX

//Menu Info. These are global
int selection = 0;

//PID tune parameters for each circuit
double kp_Marx = 50;
double ki_Marx = 0;
double kd_Marx = 25;
double kp_MTG = 15;
double ki_MTG = 0;
double kd_MTG = 10;
double kp_Switch = 50;
double ki_Switch = 0;
double kd_Switch = 25;
double kp_SwitchTG70 = 15;
double ki_SwitchTG70 = 0;
double kd_SwitchTG70 = 10;
double kp_MarxTG70 = 15;
double ki_MarxTG70 = 0;
double kd_MarxTG70 = 10;

//Boolean for if SD card is present
bool sdCard = false;
  
//Create variables for storing state of buttons and starting conditions for buttons, these are global
bool marxenableState = false;
bool mtgenableState = false;
bool switchenableState = false;
bool tg70switchenableState = false;
bool tg70marxenableState = false;
bool alarmState = false;
bool automatereclaimerState = true;
bool purgeState = false;
bool shotmodeState = false;
bool startreclaimerState = false;
bool stopreclaimerState = false;
bool abortState = false;

bool lastmarxenableState = false;
bool lastmtgenableState = false;
bool lastswitchenableState = false;
bool lasttg70switchenableState = false;
bool lasttg70marxenableState = false;
bool lastalarmState = false;
bool lastautomatereclaimerState = false;
bool lastpurgeState = false;
bool lastshotmodeState = false;
bool laststartreclaimerState = false;
bool laststopreclaimerState = false;
bool lastabortState = false;

//Error parameters
bool alarmEnable = false;
bool errorState = false;
unsigned long previousTime = 0;

//Timeout states for circuits.
long int marxmaxTime = 5000;
long int mtgmaxTime = 5000;
long int switchmaxTime = 5000;
long int tg70switchmaxTime = 5000;
long int tg70marxmaxTime = 5000;

//Delay time for each circuit
long int marxDelay = 1000;
long int mtgDelay = 1000;
long int switchDelay = 1000;
long int tg70marxDelay = 1000;
long int tg70switchDelay = 1000;

//Circuit check time delays
unsigned long checkMarxTime = 0;
unsigned long checkMTGTime = 0;
unsigned long checkSwitchTime = 0;
unsigned long checkTG70SwitchTime = 0;
unsigned long checkTG70MarxTime = 0;

//Purge time. Default is 2 minutes
long int marxPurgeTime = 120000;
long int mtgPurgeTime = 120000;
long int switchPurgeTime = 120000;
long int tg70switchPurgeTime = 120000;
long int tg70marxPurgeTime = 120000;

//Minimum bottle pressure
double minBottlePressure = 50.00;

//Standby mode.
bool standbyMode = true;

//Cotrol mode.
bool automaticMode = true;

//State of reclaimer.
bool reclaimerRunning = false;
int reclaimerSafetyTime = 30000;
unsigned long int previousReclaimerSafetyTime = 0;

//Setpoints
double Marxsetpoint = 100.00;
double MTGsetpoint = 100.00;
double Switchsetpoint = 100.00;
double TG70Switchsetpoint = 100.00;
double TG70Marxsetpoint = 100.00;
double maxReclaimerPressure = 500.0;
double minReclaimerPressure = 50.0;

#define C_NUM_CIRCUITS 7
#define MOD(a, b) (((a)%(b))<0?((a)%(b)+b):((a)%(b)))
#define CIRC_IDX(n) (MOD((n),C_NUM_CIRCUITS))
#define FLOAT_DEC(f,n) (((f) - (int)(f)) * pow(10,(n)))

// CIRCUIT STRING MAP
const char *circuit_map[] = {
  "MARX",
  "MARX TG70",
  "MTG",
  "SWITCH",
  "SWITCH TG70",
  "RECLAIMER",
  "MIN. SUPPLY"
};

// CIRCUIT CALIBRATION COEFFICIENTS
double Marxcalibration = 20.00;
double MTGcalibration = 20.00;
double Switchcalibration = 20.00;
double TG70Switchcalibration = 20.00;
double TG70Marxcalibration = 20.00;
double Reclaimcalibration = 20.00;
double Minsupplycalibration = 20.00;

// CIRCUIT CALIBRATION MAP
double *calibration_map[] = {
  &Marxcalibration,
  &TG70Marxcalibration,
  &MTGcalibration,
  &Switchcalibration,
  &TG70Switchcalibration,
  &Reclaimcalibration,
  &Minsupplycalibration,
};

// CIRCUIT ANALOG PIN MAP
int analog_map[] = { A0, A3, A2, A1, A4, A5, A6 };

//Checksum setup
CRC8 crc;

//Brightness
int brightness = 100;

/*
//Preset Names
String preset1 = "";
String preset2 = "";
String preset3 = "";
String preset4 = "";
String preset5 = "";
String preset6 = "";
*/

//Background color
int color = 0;

//-------------------------------------------------------------------------------------------------------------
//Unit Tests
//-------------------------------------------------------------------------------------------------------------

test(serial0Communications)
{
  int expected = 0;

  int result = Serial.available();

  assertEqual(result, expected);
}

test(serial3Communications)
{
  int expected = 0;

  int result = Serial3.available();

  assertEqual(result, expected);
}

test(SDCardReader)
{
  int expected = 1;

  int result = SD.begin(csPin);

  assertEqual(result, expected);
}

test(Checksum)
{
  char *oldSettings[2] = {"hi", "hello"};
  char *currentSettings[2] = {"hi", "hello"};

  crc.reset();

  int old = 0, current = 0, i = 0;

  while(i < sizeof(oldSettings)/sizeof(int))
  {
    old = calcCRC8((uint8_t *)oldSettings[i], 9);
    current = calcCRC8((uint8_t *)currentSettings[i], 9);
    i++;

    assertEqual(old, current);
  }
}


//-------------------------------------------------------------------------------------------------------------
//Setup
//-------------------------------------------------------------------------------------------------------------
void setup() 
{
  Serial.begin(9600); //This is just for debugging
  Serial3.begin(9600);
  myNex.begin(9600);  //For touchscreen comms

  SD.begin();
  SD.remove("Log.txt");

  //Start the initial boot screen
  myNex.writeStr("page Boot_Page");
  myNex.writeStr("bootText.txt", "INITIALIZING...\r\n");
  sysLog(millis(), "INITIALIZING.");
  myNex.writeNum("Progress_Bar.val", 0);

  //Unit test setup
  myNex.writeStr("bootText.txt+", "Setting up unit tests...\r\n");
  sysLog(millis(), "Setting up unit tests.");
  aunit::TestRunner::setPrinter(&Serial);  //I would like to set this to output onto the screen eventually
  aunit::TestRunner::setTimeout(10);
  myNex.writeNum("Progress_Bar.val", 10);
    
  //Set alarm pin to OUTPUT mode.
  pinMode(alarmsoundPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "Alarm pin set.\r\n");
  sysLog(millis(), "Alarm pin set.");
  myNex.writeNum("Progress_Bar.val", 20);

  //Set solenoid pins to OUTPUT mode.
  pinMode(marxinPin, OUTPUT);
  pinMode(marxoutPin, OUTPUT);
  pinMode(mtginPin, OUTPUT);
  pinMode(mtgoutPin, OUTPUT);
  pinMode(switchinPin, OUTPUT);
  pinMode(switchoutPin, OUTPUT);
  pinMode(tg70switchinPin, OUTPUT);
  pinMode(tg70switchoutPin, OUTPUT);   
  pinMode(tg70marxinPin, OUTPUT);
  pinMode(tg70marxoutPin, OUTPUT);
  pinMode(reclaimerstartPin, OUTPUT);
  pinMode(reclaimerstopPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "Solenoid pins set.\r\n");
  sysLog(millis(), "Solenoid pins set.");
  myNex.writeNum("Progress_Bar.val", 30);

  //Set SDcard pin to OUTPUT mode.
  pinMode(csPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "SD pin set.\r\n");  //Need to write to log also
  sysLog(millis(), "SD pin set.");
  myNex.writeNum("Progress_Bar.val", 40);

  //Set LED pins to OUTPUT mode.
  pinMode(marxenableLEDPin, OUTPUT);
  pinMode(mtgenableLEDPin, OUTPUT);
  pinMode(switchenableLEDPin, OUTPUT);
  pinMode(tg70switchenableLEDPin, OUTPUT);
  pinMode(tg70marxenableLEDPin, OUTPUT);
  pinMode(shotmodeLEDPin, OUTPUT);
  pinMode(purgeLEDPin, OUTPUT);
  pinMode(alarmLEDPin, OUTPUT);
  pinMode(automatereclaimerLEDPin, OUTPUT);
  pinMode(startLEDPin, OUTPUT);
  pinMode(stopLEDPin, OUTPUT);
  pinMode(abortLEDPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "LED pins set.\r\n");
  sysLog(millis(), "LED pins set.");
  myNex.writeNum("Progress_Bar.val", 50);

  //Set button pins to INPUT mode.
  pinMode(shotmodePin, INPUT);
  pinMode(purgePin, INPUT);
  pinMode(alarmPin, INPUT);
  pinMode(automatereclaimerPin, INPUT);
  pinMode(marxenablePin, INPUT);
  pinMode(mtgenablePin, INPUT);
  pinMode(switchenablePin, INPUT);
  pinMode(tg70marxenablePin, INPUT);
  pinMode(tg70switchenablePin, INPUT);
  pinMode(reclaimerstartenablePin, INPUT);
  pinMode(reclaimerstopenablePin, INPUT);
  pinMode(abortbuttonPin, INPUT);
  myNex.writeStr("bootText.txt+", "Button pins set.\r\n");
  sysLog(millis(), "Setting up unit tests.");
  myNex.writeNum("Progress_Bar.val", 60);

  //Turn on the internal pullup resistor on all buttons.
  pinMode(shotmodePin, INPUT_PULLUP);
  pinMode(purgePin, INPUT_PULLUP);
  pinMode(alarmPin, INPUT_PULLUP);
  pinMode(automatereclaimerPin, INPUT_PULLUP);
  pinMode(marxenablePin, INPUT_PULLUP);
  pinMode(mtgenablePin, INPUT_PULLUP);
  pinMode(switchenablePin, INPUT_PULLUP);
  pinMode(tg70marxenablePin, INPUT_PULLUP);
  pinMode(tg70switchenablePin, INPUT_PULLUP);
  pinMode(reclaimerstartenablePin, INPUT_PULLUP);
  pinMode(reclaimerstopenablePin, INPUT_PULLUP);
  pinMode(abortbuttonPin, INPUT_PULLUP);
  myNex.writeStr("bootText.txt+", "Button pullup resistors activated.\r\n");
  sysLog(millis(), "Button pullup resistors activated.");
  myNex.writeNum("Progress_Bar.val", 70);

  //Startup process. Load the last used settings. If none are found, create some defaults.
  //Check if SD card exists
  myNex.writeStr("bootText.txt+", "Checking SD Card...\r\n");
  sysLog(millis(), "Checking SD Card.");
  myNex.writeNum("Progress_Bar.val", 80);
  if(!SD.begin())//No sd card is found. Set the circuit pressure to whatever they happen to be at the time
  { 
    sdCard = false;
    myNex.writeStr("bootText.txt+", "WARNING: SD card not found!\r\n");
    sysLog(millis(), "WARNING: SD card not found!");
    myNex.writeNum("Progress_Bar.val", 90);
    Marxsetpoint = analogRead(marxanaloginPin);                    
    MTGsetpoint = analogRead(mtganaloginPin);
    Switchsetpoint = analogRead(switchanaloginPin);
    TG70Switchsetpoint = analogRead(tg70switchanaloginPin);
    TG70Marxsetpoint = analogRead(tg70marxanaloginPin);
    delay(1500);
  }
  else
  {
    myNex.writeStr("bootText.txt+", "Loading previous settings...\r\n");
    sysLog(millis(), "Loading previous settings.");
    sdCard = true;
    if(SD.exists("Setting.txt")) //Previous settings are found. Load the previous settings into the controller.
    {
      File previousSettingFile = SD.open("Setting.txt", FILE_READ);
      alarmEnable = previousSettingFile.readStringUntil('\n').toInt();
      Marxsetpoint = previousSettingFile.readStringUntil('\n').toDouble();
      MTGsetpoint = previousSettingFile.readStringUntil('\n').toDouble();
      Switchsetpoint = previousSettingFile.readStringUntil('\n').toDouble();
      TG70Switchsetpoint = previousSettingFile.readStringUntil('\n').toDouble();
      TG70Marxsetpoint = previousSettingFile.readStringUntil('\n').toDouble();
      maxReclaimerPressure = previousSettingFile.readStringUntil('\n').toDouble();
      minReclaimerPressure = previousSettingFile.readStringUntil('\n').toDouble();
      marxenableState = previousSettingFile.readStringUntil('\n').toInt();
      mtgenableState = previousSettingFile.readStringUntil('\n').toInt();
      switchenableState = previousSettingFile.readStringUntil('\n').toInt();
      tg70switchenableState = previousSettingFile.readStringUntil('\n').toInt();
      tg70marxenableState = previousSettingFile.readStringUntil('\n').toInt();
      marxmaxTime = previousSettingFile.readStringUntil('\n').toInt();
      mtgmaxTime = previousSettingFile.readStringUntil('\n').toInt();
      switchmaxTime = previousSettingFile.readStringUntil('\n').toInt();
      tg70switchmaxTime = previousSettingFile.readStringUntil('\n').toInt();
      tg70marxmaxTime = previousSettingFile.readStringUntil('\n').toInt();
      marxDelay = previousSettingFile.readStringUntil('\n').toInt();
      mtgDelay = previousSettingFile.readStringUntil('\n').toInt();
      switchDelay = previousSettingFile.readStringUntil('\n').toInt();
      tg70marxDelay = previousSettingFile.readStringUntil('\n').toInt();
      tg70switchDelay = previousSettingFile.readStringUntil('\n').toInt();
      marxPurgeTime = previousSettingFile.readStringUntil('\n').toInt();
      mtgPurgeTime = previousSettingFile.readStringUntil('\n').toInt();
      switchPurgeTime = previousSettingFile.readStringUntil('\n').toInt();
      tg70switchPurgeTime = previousSettingFile.readStringUntil('\n').toInt();
      tg70marxPurgeTime = previousSettingFile.readStringUntil('\n').toInt();
      minBottlePressure = previousSettingFile.readStringUntil('\n').toDouble();
      kp_Marx = previousSettingFile.readStringUntil('\n').toDouble();
      ki_Marx = previousSettingFile.readStringUntil('\n').toDouble();
      kd_Marx = previousSettingFile.readStringUntil('\n').toDouble();
      kp_MTG = previousSettingFile.readStringUntil('\n').toDouble();
      ki_MTG = previousSettingFile.readStringUntil('\n').toDouble();
      kd_MTG = previousSettingFile.readStringUntil('\n').toDouble();
      kp_Switch = previousSettingFile.readStringUntil('\n').toDouble();
      ki_Switch = previousSettingFile.readStringUntil('\n').toDouble();
      kd_Switch = previousSettingFile.readStringUntil('\n').toDouble();
      kp_SwitchTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      ki_SwitchTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      kd_SwitchTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      kp_MarxTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      ki_MarxTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      kd_MarxTG70 = previousSettingFile.readStringUntil('\n').toDouble();
      reclaimerSafetyTime = previousSettingFile.readStringUntil('\n').toInt();
      
      Marxcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      MTGcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      Switchcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      TG70Switchcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      TG70Marxcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      Reclaimcalibration = previousSettingFile.readStringUntil('\n').toDouble();
      Minsupplycalibration = previousSettingFile.readStringUntil('\n').toDouble();
      
      lastmarxenableState = !marxenableState;
      lastmtgenableState = !mtgenableState;
      lastswitchenableState = !switchenableState;
      lasttg70switchenableState = !tg70switchenableState;
      lasttg70marxenableState = !tg70marxenableState;
      previousSettingFile.close();
      readPresets();

      SaveCurrentSettings();
      myNex.writeStr("bootText.txt+", "Previous settings loaded successfully!\r\n");
      sysLog(millis(), "Previous settings loaded successfully!");
      myNex.writeNum("Progress_Bar.val", 90);
      delay(1500);
    }
    else //No previous settings are found. Set the circuit pressure to whatever they happen to be at the time
    {
      myNex.writeStr("bootText.txt+", "No previous settings found. Using default settings...\r\n");
      sysLog(millis(), "No previous settings found. Using default settings.");
      myNex.writeNum("Progress_Bar.val", 90);
      Marxsetpoint = analogRead(marxanaloginPin);                    
      MTGsetpoint = analogRead(mtganaloginPin);
      Switchsetpoint = analogRead(switchanaloginPin);
      TG70Switchsetpoint = analogRead(tg70switchanaloginPin);
      TG70Marxsetpoint = analogRead(tg70marxanaloginPin);
      delay(1500);
    } 
  }
  
  myNex.writeStr("bootText.txt+", "Boot complete!\r\n");
  sysLog(millis(), "Boot complete.");
  myNex.writeNum("Progress_Bar.val", 100);
  delay(3000);
  myNex.writeStr("page Main_Menu");
}


//-------------------------------------------------------------------------------------------------------------
//Main loop
//-------------------------------------------------------------------------------------------------------------
void loop() 
{
  aunit::TestRunner::run();  //Run unit tests

  //Check the state of the buttons. This allows a user to press buttons at almost any time. You will see this function call everywhere.
  ControlButtonStateManager();

  //Check if a user has pressed a button on the touchscreen, and send the user to the correct function. (Vladislav Petrov)
  myNex.NextionListen();
  
 
  //Start shotmode pressure setting sequence
  if(shotmodeState && automaticMode)
  {
    ShotPressure(false);
  }

  
  //Start purge sequence for all enabled systems
  if(purgeState && automaticMode)
  {
    
    //If no circuits are enabled, display messege to user
    while(!marxenableState && !mtgenableState && !switchenableState && !tg70switchenableState && !tg70marxenableState && purgeState)
    {
      ControlButtonStateManager();
      //lcd.print("ENABLE CIRCUITS ");   //Will be used for LOG FUNCTION
      //lcd.print("                ");   //Will be used for LOG FUNCTION
    }

    //Flags to check if an enabled circuit has been purged or not
    bool marxFlag = true;
    bool MTGFlag = true;
    bool switchFlag = true;
    bool tg70switchFlag = true;
    bool tg70marxFlag = true;

    //Loop through each enabled circuit and purge. Set a flag for purged circuits if a user has added more mid loop, as to not repeat them
    while(purgeState && (marxFlag || MTGFlag || switchFlag || tg70switchFlag || tg70marxFlag))
    {
      ControlButtonStateManager();

      if(tg70marxenableState && purgeState && tg70marxFlag)
      {
        tg70marxFlag = false;
        Purge(tg70marxenableState, tg70marxinPin, tg70marxoutPin);
      }

      else if(tg70switchenableState && purgeState && tg70switchFlag)
      {
        tg70switchFlag = false;
        Purge(tg70switchenableState, tg70switchinPin, tg70switchoutPin);
      }

      else if(mtgenableState && purgeState && MTGFlag)
      {
        MTGFlag = false;
        Purge(mtgenableState, mtginPin, mtgoutPin);
      }
      
      else if(marxenableState && purgeState && marxFlag)
      {
        marxFlag = false;
        Purge(marxenableState, marxinPin, marxoutPin);
      }

      else if(switchenableState  && purgeState && switchFlag)
      {
        switchFlag = false;
        Purge(switchenableState, switchinPin, switchoutPin);
      }

      else
      {
        break;
      }
    }
    
    //Raise enabled cicuits to half pressure if shotmode state is false
    if(!shotmodeState) 
    {
      ShotPressure(true);
    }

    //Display messege to user that the purge is completed
    if(marxenableState || mtgenableState || switchenableState || tg70switchenableState || tg70marxenableState)
    {
      //lcd.print("PURGE COMPLETE  ");   //Will be used for LOG FUNCTION
      //lcd.print("                ");   //Will be used for LOG FUNCTION
      delay(3000);
    }
    purgeState = false;
    standbyMode = true;
  }
  //Start abort pressure setting sequence
  if(abortState && automaticMode)
  {
    abortShot();
  }
}


//-------------------------------------------------------------------------------------------------------------
//Function for setting shot pressures
//-------------------------------------------------------------------------------------------------------------
void ShotPressure(bool half)
{
  
  //Create setpoint range
  double range = 5.00;
  
  //Create division coeficiant for standby mode
  double divisor = 2.00;

  //Create small acceptable range around setpoints
  double MarxHIGH = Marxsetpoint + range, MarxLOW = Marxsetpoint - range;
  double MTGHIGH = MTGsetpoint + range, MTGLOW = MTGsetpoint - range;
  double SwitchHIGH = Switchsetpoint + range, SwitchLOW = Switchsetpoint - range;
  double TG70SwitchHIGH = TG70Switchsetpoint + range, TG70SwitchLOW = TG70Switchsetpoint - range;
  double TG70MarxHIGH = TG70Marxsetpoint + range, TG70MarxLOW = TG70Marxsetpoint - range;
  
  //Set the acceptable range around setpoints when in standby mode
  if(half)
  {
    MarxHIGH = Marxsetpoint/divisor + range; 
    MarxLOW = Marxsetpoint/divisor - range;
    MTGHIGH = MTGsetpoint/divisor + range; 
    MTGLOW = MTGsetpoint/divisor - range;
    SwitchHIGH = Switchsetpoint/divisor + range; 
    SwitchLOW = Switchsetpoint/divisor - range;
    TG70SwitchHIGH = TG70Switchsetpoint/divisor + range; 
    TG70SwitchLOW = TG70Switchsetpoint/divisor - range;
    TG70MarxHIGH = TG70Marxsetpoint/divisor + range; 
    TG70MarxLOW = TG70Marxsetpoint/divisor - range;
  }
  
  
  //Create input/output variables for PID functions
  double Marxinput,MTGinput,Switchinput,TG70Switchinput,TG70Marxinput;
  double Marxincreaseoutput,MTGincreaseoutput,Switchincreaseoutput,TG70Switchincreaseoutput,TG70Marxincreaseoutput;
  double Marxdecreaseoutput,MTGdecreaseoutput,Switchdecreaseoutput,TG70Switchdecreaseoutput,TG70Marxdecreaseoutput;
  
  //Crease PID functions, one for pressure increase and one for pressure decrease
  PID MarxIncreasePID(&Marxinput, &Marxincreaseoutput, &Marxsetpoint, kp_Marx, ki_Marx, kd_Marx, DIRECT);
  PID MarxDecreasePID(&Marxinput, &Marxdecreaseoutput, &Marxsetpoint, kp_Marx, ki_Marx, kd_Marx, REVERSE); 
  PID MTGIncreasePID(&MTGinput, &MTGincreaseoutput, &MTGsetpoint, kp_MTG, ki_MTG, kd_MTG, DIRECT);
  PID MTGDecreasePID(&MTGinput, &MTGdecreaseoutput, &MTGsetpoint, kp_MTG, ki_MTG, kd_MTG, REVERSE); 
  PID SwitchIncreasePID(&Switchinput, &Switchincreaseoutput, &Switchsetpoint, kp_Switch, ki_Switch, kd_Switch, DIRECT);
  PID SwitchDecreasePID(&Switchinput, &Switchdecreaseoutput, &Switchsetpoint, kp_Switch, ki_Switch, kd_Switch, REVERSE); 
  PID TG70SwitchIncreasePID(&TG70Switchinput, &TG70Switchincreaseoutput, &TG70Switchsetpoint, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, DIRECT);
  PID TG70SwitchDecreasePID(&TG70Switchinput, &TG70Switchdecreaseoutput, &TG70Switchsetpoint, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, REVERSE); 
  PID TG70MarxIncreasePID(&TG70Marxinput, &TG70Marxincreaseoutput, &TG70Marxsetpoint, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, DIRECT);
  PID TG70MarxDecreasePID(&TG70Marxinput, &TG70Marxdecreaseoutput, &TG70Marxsetpoint, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, REVERSE); 
  
  //Set PIDs to automatic
  MarxIncreasePID.SetMode(AUTOMATIC);
  MarxDecreasePID.SetMode(AUTOMATIC);
  MTGIncreasePID.SetMode(AUTOMATIC);
  MTGDecreasePID.SetMode(AUTOMATIC);
  SwitchIncreasePID.SetMode(AUTOMATIC);
  SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70SwitchIncreasePID.SetMode(AUTOMATIC);
  TG70SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70MarxIncreasePID.SetMode(AUTOMATIC);
  TG70MarxDecreasePID.SetMode(AUTOMATIC);
   
  //Set maximum solenoid open time(in Milliseconds)
  const int WindowSize = 5000;
  
  //Set upper and lower limits on how long a solenoid can open for
  MarxIncreasePID.SetOutputLimits(0, WindowSize);
  MarxDecreasePID.SetOutputLimits(0, WindowSize);
  MTGIncreasePID.SetOutputLimits(0, WindowSize);
  MTGDecreasePID.SetOutputLimits(0, WindowSize);
  SwitchIncreasePID.SetOutputLimits(0, WindowSize);
  SwitchDecreasePID.SetOutputLimits(0, WindowSize);
  TG70SwitchIncreasePID.SetOutputLimits(0, WindowSize);
  TG70SwitchDecreasePID.SetOutputLimits(0, WindowSize);
  TG70MarxIncreasePID.SetOutputLimits(0, WindowSize);
  TG70MarxDecreasePID.SetOutputLimits(0, WindowSize);
  
  //Variables for storing pressure values.
  double Marxcurrentpressure,MTGcurrentpressure,Switchcurrentpressure,TG70Switchcurrentpressure,TG70Marxcurrentpressure;
  
  //While in shotmode loop and ensure pressures are within tolerance
  while((shotmodeState && !purgeState) || (purgeState && half))
  {
    
    //Check the state of the front panel buttons
    ControlButtonStateManager();
    
    //Check the pressures 
    Marxcurrentpressure = analogRead(marxanaloginPin);                    
    MTGcurrentpressure = analogRead(mtganaloginPin);
    Switchcurrentpressure = analogRead(switchanaloginPin);
    TG70Switchcurrentpressure = analogRead(tg70switchanaloginPin);
    TG70Marxcurrentpressure = analogRead(tg70marxanaloginPin);
  
    //If no circuits are enabled, display a messege to the user to turn on a circuit
    while(!marxenableState && !mtgenableState && !switchenableState && !tg70switchenableState && !tg70marxenableState && shotmodeState && !half)
    {
      ControlButtonStateManager();
      //lcd.print("ENABLE CIRCUITS ");   //Will be used for LOG FUNCTION
      //lcd.print("                ");   //Will be used for LOG FUNCTION
    }
  
    
    //Check If pressures are too low, if so call RaisePressure for each system that is low and enabled.
    if (Marxcurrentpressure < MarxLOW && marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkMarxTime >= marxDelay)
      {
        RaisePressure(marxenableState, divisor, half, Marxcurrentpressure, Marxsetpoint, marxinPin, marxanaloginPin, WindowSize, Marxinput, Marxincreaseoutput, MarxIncreasePID);
        checkMarxTime = millis(); 
      }
    }
  
    if (MTGcurrentpressure < MTGLOW && mtgenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkMTGTime >= mtgDelay)
      {
        RaisePressure(mtgenableState, divisor, half, MTGcurrentpressure, MTGsetpoint, mtginPin, mtganaloginPin, WindowSize, MTGinput, MTGincreaseoutput, MTGIncreasePID);
        checkMTGTime = millis();
      }
    }
  
    if (Switchcurrentpressure < SwitchLOW && switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkSwitchTime >= switchDelay)
      {
        RaisePressure(switchenableState, divisor, half, Switchcurrentpressure, Switchsetpoint, switchinPin, switchanaloginPin, WindowSize, Switchinput, Switchincreaseoutput, SwitchIncreasePID);
        checkSwitchTime = millis();
      }
    }
 
    if (TG70Switchcurrentpressure < TG70SwitchLOW && tg70switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkTG70SwitchTime >= tg70switchDelay)
      {
        RaisePressure(tg70switchenableState, divisor, half, TG70Switchcurrentpressure, TG70Switchsetpoint, tg70switchinPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchincreaseoutput, TG70SwitchIncreasePID);
        checkTG70SwitchTime = millis();
      }
    }
  
    if (TG70Marxcurrentpressure < TG70MarxLOW && tg70marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkTG70MarxTime >= tg70marxDelay)
      {
        RaisePressure(tg70marxenableState, divisor, half, TG70Marxcurrentpressure, TG70Marxsetpoint, tg70marxinPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxincreaseoutput, TG70MarxIncreasePID);
        checkTG70MarxTime = millis();
      }
    }
  
    //Check if pressures are too high, if so call ReducePressure for each system that is high and enabled. 
    if (Marxcurrentpressure > MarxHIGH && marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkMarxTime >= marxDelay)
      {
        ReducePressure(marxenableState, divisor, half, Marxcurrentpressure, Marxsetpoint, marxoutPin, marxanaloginPin, WindowSize, Marxinput, Marxdecreaseoutput, MarxDecreasePID);
        checkMarxTime = millis();
      }
    }
  
    if (MTGcurrentpressure > MTGHIGH && mtgenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkMTGTime >= mtgDelay)
      {
        ReducePressure(mtgenableState, divisor, half, MTGcurrentpressure, MTGsetpoint, mtgoutPin, mtganaloginPin, WindowSize, MTGinput, MTGdecreaseoutput, MTGDecreasePID);
        checkMTGTime = millis();
      }
    }
    
    if (Switchcurrentpressure > SwitchHIGH && switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkSwitchTime >= switchDelay)
      {
        ReducePressure(switchenableState, divisor, half, Switchcurrentpressure, Switchsetpoint, switchoutPin, switchanaloginPin,WindowSize, Switchinput, Switchdecreaseoutput, SwitchDecreasePID);
        checkSwitchTime = millis();
      }
    }
  
    if (TG70Switchcurrentpressure > TG70SwitchHIGH && tg70switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkTG70SwitchTime >= tg70switchDelay)
      {
        ReducePressure(tg70switchenableState, divisor, half, TG70Switchcurrentpressure, TG70Switchsetpoint, tg70switchoutPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchdecreaseoutput, TG70SwitchDecreasePID);
        checkTG70SwitchTime = millis();
      }
    }
    
    if (TG70Marxcurrentpressure > TG70MarxHIGH && tg70marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      if(millis() - checkTG70MarxTime >= tg70marxDelay)
      {
        ReducePressure(tg70marxenableState, divisor, half, TG70Marxcurrentpressure, TG70Marxsetpoint, tg70marxoutPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxdecreaseoutput, TG70MarxDecreasePID);
        checkTG70MarxTime = millis();
      }
    }
    
    // If all pressure is in correct range
    else
    {      
      //In shot mode, simple wait and adjust pressure if needed
      if(!half && ((millis() - checkMarxTime >= marxDelay * 2) && (millis() - checkMTGTime >= mtgDelay * 2) && (millis() - checkSwitchTime >= switchDelay * 2) && (millis() - checkTG70SwitchTime >= tg70switchDelay * 2) && (millis() - checkTG70MarxTime >= tg70marxDelay * 2)))
      {
        ControlButtonStateManager();
        //lcd.print("CIRCUITS AT     ");   //Will be used for LOG FUNCTION
        //lcd.print("SET PRESSURE    ");   //Will be used for LOG FUNCTION
      }
      //In standby mode, break the loop when pressure is set
      else if( half && ((millis() - checkMarxTime >= marxDelay * 2) && (millis() - checkMTGTime >= mtgDelay * 2) && (millis() - checkSwitchTime >= switchDelay * 2) && (millis() - checkTG70SwitchTime >= tg70switchDelay * 2) && (millis() - checkTG70MarxTime >= tg70marxDelay * 2)))
      {
        ControlButtonStateManager();
        //lcd.print("CIRCUITS AT     ");   //Will be used for LOG FUNCTION
        //lcd.print("HALF PRESSURE   ");   //Will be used for LOG FUNCTION
        delay(3000);
        break;
      }
      //Still setting pressure
      else
      {
        ControlButtonStateManager();
        //lcd.print("SETTING PRESSURE");   //Will be used for LOG FUNCTION
      }
    }
  }

  //Ensure all solenoids are closed
  digitalWrite(marxinPin, LOW);
  digitalWrite(mtginPin, LOW);
  digitalWrite(switchinPin, LOW);
  digitalWrite(tg70switchinPin, LOW);
  digitalWrite(tg70marxinPin, LOW);

  digitalWrite(marxoutPin, LOW);
  digitalWrite(mtgoutPin, LOW);
  digitalWrite(switchoutPin, LOW);
  digitalWrite(tg70switchoutPin, LOW);
  digitalWrite(tg70marxoutPin, LOW);
}


//-------------------------------------------------------------------------------------------------------------
//Function for aborting shot and lowering enabled circuit pressure to half
//-------------------------------------------------------------------------------------------------------------
void abortShot()
{
  //lcd.print("ABORTING SHOT   ");   //Will be used for LOG FUNCTION
  delay(3000);

  //Create setpoint range
  double range = 5.00;

  //Create division coeficiant for standby mode
  double divisor = 2.00;

 //Create small acceptable range around setpoints
  double MarxHIGH = Marxsetpoint/divisor + range; 
  double MTGHIGH = MTGsetpoint/divisor + range; 
  double SwitchHIGH = Switchsetpoint/divisor + range; 
  double TG70SwitchHIGH = TG70Switchsetpoint/divisor + range; 
  double TG70MarxHIGH = TG70Marxsetpoint/divisor + range; 
 
  //Create input/output variables for PID functions
  double Marxinput,MTGinput,Switchinput,TG70Switchinput,TG70Marxinput;
  double Marxdecreaseoutput,MTGdecreaseoutput,Switchdecreaseoutput,TG70Switchdecreaseoutput,TG70Marxdecreaseoutput;

  //Crease PID functions
  PID MarxDecreasePID(&Marxinput, &Marxdecreaseoutput, &Marxsetpoint, kp_Marx, ki_Marx, kd_Marx, REVERSE); 
  PID MTGDecreasePID(&MTGinput, &MTGdecreaseoutput, &MTGsetpoint, kp_MTG, ki_MTG, kd_MTG, REVERSE); 
  PID SwitchDecreasePID(&Switchinput, &Switchdecreaseoutput, &Switchsetpoint, kp_Switch, ki_Switch, kd_Switch, REVERSE); 
  PID TG70SwitchDecreasePID(&TG70Switchinput, &TG70Switchdecreaseoutput, &TG70Switchsetpoint, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, REVERSE); 
  PID TG70MarxDecreasePID(&TG70Marxinput, &TG70Marxdecreaseoutput, &TG70Marxsetpoint, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, REVERSE); 

  //Set PIDs to automatic
  MarxDecreasePID.SetMode(AUTOMATIC);
  MTGDecreasePID.SetMode(AUTOMATIC);
  SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70MarxDecreasePID.SetMode(AUTOMATIC);
 
  //Set maximum solenoid open time(in Milliseconds)
  const int WindowSize = 5000;

  //Set upper and lower limits on how long a solenoid can open for
  MarxDecreasePID.SetOutputLimits(0, WindowSize);
  MTGDecreasePID.SetOutputLimits(0, WindowSize);
  SwitchDecreasePID.SetOutputLimits(0, WindowSize);
  TG70SwitchDecreasePID.SetOutputLimits(0, WindowSize);
  TG70MarxDecreasePID.SetOutputLimits(0, WindowSize);

  //Variables for storing pressure values.
  double Marxcurrentpressure,MTGcurrentpressure,Switchcurrentpressure,TG70Switchcurrentpressure,TG70Marxcurrentpressure;

  //While in abort loop and ensure pressures are within tolerance
  while(abortState)
  {
  
    //Check the state of the front panel buttons
    ControlButtonStateManager();
  
    //Check the pressures 
    Marxcurrentpressure = analogRead(marxanaloginPin);                    
    MTGcurrentpressure = analogRead(mtganaloginPin);
    Switchcurrentpressure = analogRead(switchanaloginPin);
    TG70Switchcurrentpressure = analogRead(tg70switchanaloginPin);
    TG70Marxcurrentpressure = analogRead(tg70marxanaloginPin);

  //Check if pressures are too high, if so call ReducePressure for each system that is high and enabled. 
  if (Marxcurrentpressure > MarxHIGH && marxenableState)
  {
    if(millis() - checkMarxTime >= marxDelay)
    {
      ReducePressure(marxenableState, divisor, true, Marxcurrentpressure, Marxsetpoint, marxoutPin, marxanaloginPin, WindowSize, Marxinput, Marxdecreaseoutput, MarxDecreasePID);
      checkMarxTime = millis();
    }
  }

  if (MTGcurrentpressure > MTGHIGH && mtgenableState)
  {
    if(millis() - checkMTGTime >= mtgDelay)
    {
      ReducePressure(mtgenableState, divisor, true, MTGcurrentpressure, MTGsetpoint, mtgoutPin, mtganaloginPin, WindowSize, MTGinput, MTGdecreaseoutput, MTGDecreasePID);
      checkMTGTime = millis();
    }
  }
  
  if (Switchcurrentpressure > SwitchHIGH && switchenableState)
  {
    if(millis() - checkSwitchTime >= switchDelay)
    {
      ReducePressure(switchenableState, divisor, true, Switchcurrentpressure, Switchsetpoint, switchoutPin, switchanaloginPin,WindowSize, Switchinput, Switchdecreaseoutput, SwitchDecreasePID);
      checkSwitchTime = millis();
    }
  }
  
  if (TG70Switchcurrentpressure > TG70SwitchHIGH && tg70switchenableState)
  {
    if(millis() - checkTG70SwitchTime >= tg70switchDelay)
    {
      ReducePressure(tg70switchenableState, divisor, true, TG70Switchcurrentpressure, TG70Switchsetpoint, tg70switchoutPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchdecreaseoutput, TG70SwitchDecreasePID);
      checkTG70SwitchTime = millis();
    }
  }
  
  if (TG70Marxcurrentpressure > TG70MarxHIGH && tg70marxenableState)
  {
    if(millis() - checkTG70MarxTime >= tg70marxDelay)
    {
      ReducePressure(tg70marxenableState, divisor, true, TG70Marxcurrentpressure, TG70Marxsetpoint, tg70marxoutPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxdecreaseoutput, TG70MarxDecreasePID);
      checkTG70MarxTime = millis();
    }
  }

  //Check is pressure is reached
  else
  {      
    //In shot mode, simple wait and adjust pressure if needed
    if((millis() - checkMarxTime >= marxDelay * 2) && (millis() - checkMTGTime >= mtgDelay * 2) && (millis() - checkSwitchTime >= switchDelay * 2) && (millis() - checkTG70SwitchTime >= tg70switchDelay * 2) && (millis() - checkTG70MarxTime >= tg70marxDelay * 2))
    {
      ControlButtonStateManager();
      //lcd.print("CIRCUITS AT     ");   //Will be used for LOG FUNCTION
      //lcd.print("HALF PRESSURE   ");   //Will be used for LOG FUNCTION
      delay(3000);
      break;
    }
    //Still setting pressure
    else
    {
      ControlButtonStateManager();
      //lcd.print("SETTING TO      ");   //Will be used for LOG FUNCTION
      //lcd.print("HALF PRESSURE   ");   //Will be used for LOG FUNCTION
    }
  }
  }

  //Ensure all solenoids are closed
  digitalWrite(marxinPin, LOW);
  digitalWrite(mtginPin, LOW);
  digitalWrite(switchinPin, LOW);
  digitalWrite(tg70switchinPin, LOW);
  digitalWrite(tg70marxinPin, LOW);

  digitalWrite(marxoutPin, LOW);
  digitalWrite(mtgoutPin, LOW);
  digitalWrite(switchoutPin, LOW);
  digitalWrite(tg70switchoutPin, LOW);
  digitalWrite(tg70marxoutPin, LOW);

  //Exit the abort state once pressure is set
  abortState = false;
}


//-------------------------------------------------------------------------------------------------------------
//Function for raising pressure using PID control
//-------------------------------------------------------------------------------------------------------------
void RaisePressure(bool& circuitState, double divisor, bool half, double &currentpressure, double Threshold,const int relayPin,const int analogPin,const int WindowSize, double& input, double& output,  PID& System)
{
  String circuitName;
  int maxTime;

  //initialize pressure
  double pressure = currentpressure;
  double startPressure = pressure;

  //If standby mode, reduce the threshold by the coeficiant.
  if(half)
  {
    Threshold = Threshold / divisor;
  }

  //Start the window timer
  unsigned long windowstarttime;
  windowstarttime = millis();

  //Loop while the pressure is too low 
  while ((pressure < Threshold) && (shotmodeState || purgeState) && circuitState)
    {
      //Check the states of the front panel buttons
      ControlButtonStateManager();

      //Check pressure
      pressure = analogRead(analogPin);

      //Display to the user the current status
      if(half)
      {
        //lcd.print("RAISING TO HALF:");   //Will be used for LOG FUNCTION
      }
      else
      {
        //lcd.print("RAISING:        ");   //Will be used for LOG FUNCTION             
      }
      switch(relayPin)
      {  
        case 23:
          circuitName = "MARX";
          maxTime = marxmaxTime;
          break;

        case 25:
          circuitName = "MTG";
          maxTime = mtgmaxTime;
          break;

        case 27:
          circuitName = "SWITCH";
          maxTime = switchmaxTime;
          break;

        case 29:
          circuitName = "SWITCH TG70";
          maxTime = tg70switchmaxTime;
          break;

        case 31:
          circuitName = "MARX TG70";
          maxTime = tg70marxmaxTime;
          break;
      }            
      //lcd.print(String(circuitName + "                "));   //Will be used for LOG FUNCTION
      
      //Read pressure for input to PID
      input = analogRead(analogPin);

      //Use PID to compute how long solenoid should open for
      System.Compute();

      //Read current time
      unsigned long now = millis();

      //Adjust time window if necessary
      if((now - windowstarttime > WindowSize))
      { 
        windowstarttime += WindowSize;
      }

      //Too much time has passed and pressure is still not reached. Close solenoids and set error state for a leak.
      if((now - windowstarttime >= maxTime) && (startPressure >= analogRead(analogPin)) && maxTime >= 1)
      { 
        if(startPressure == analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " CLOSED                "));
          break;
        }
        if(startPressure > analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " LEAK              "));
          break;
        }
      }      

      //If PID output is greater than current time minus the window time then the filling solenoid should be open 
      if((output > now - windowstarttime) && circuitState) 
      {
        digitalWrite(relayPin,HIGH);
      }
      
      //Otherwise the filling solenoid has been open long enough and should close
      else
      {
        digitalWrite(relayPin,LOW);
        break;
      }    
    }

   //Ensure the filling solenoid is closed
  digitalWrite(relayPin,LOW);
}


//-------------------------------------------------------------------------------------------------------------
//Function for lowering the pressure using PID control
//-------------------------------------------------------------------------------------------------------------
void ReducePressure(bool& circuitState, double divisor, bool half, double &currentpressure, double Threshold,const int relayPin, const int analogPin,const int WindowSize, double& input, double& output,  PID& System)
{     
  String circuitName;
  int maxTime;

  //Initialize the pressure
  double pressure = currentpressure;
  double startPressure = pressure;
  
  //If standby mode, reduce the threshold by the coeficiant.
  if(half)
  {
    Threshold = Threshold / divisor;
  }

  //Start the window timer
  unsigned long windowstarttime;
  windowstarttime = millis();

  //Read pressure for input to PID
  input = analogRead(analogPin);

  //Loop while pressure is too high
  while (((pressure > Threshold) && (shotmodeState || purgeState) && circuitState) || abortState)
    {
      //Check the states of the front panel buttons
      ControlButtonStateManager();
          
      //Display to the user the current status
      if(half)
      {
        //lcd.print("LOWERING TO HALF:");  //Will be used for LOG FUNCTION
      }
      else
      {
        //lcd.print("LOWERING:       ");       //Will be used for LOG FUNCTION       
      }
      switch(relayPin)
      {
        case 22:
          circuitName = "MARX";
          maxTime = marxmaxTime;
          break;

        case 24:
          circuitName = "MTG";
          maxTime = mtgmaxTime;
          break;

        case 26:
          circuitName = "SWITCH";
          maxTime = switchmaxTime;
          break;

        case 28:
          circuitName = "SWITCH TG70";
          maxTime = tg70switchmaxTime;
          break;

        case 30:
          circuitName = "MARX TG70";
          maxTime = tg70marxmaxTime;
          break;
      }            
      //lcd.print(String(circuitName + "                "));       //Will be used for LOG FUNCTION

      //Use PID to compute how long solenoid should open for
      System.Compute();

      //Read current time
      unsigned long now = millis();

      //Adjust time window if necessary
      if((now - windowstarttime > WindowSize))
      { 
        windowstarttime += WindowSize;
      }

      //Too much time has passed and pressure is still not reached. Close solenoids and set error state for a leak.
      if((now - windowstarttime >= maxTime) && (startPressure <= analogRead(analogPin)) && maxTime >= 1)
      { 
        if(startPressure == analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " CLOSED                "));
          break;
        }
        if(startPressure < analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " LEAK              "));
          break;
        }
      }  

      //If PID output is greater than current time minus the window time then the emptying solenoid should be open 
      if((output > now - windowstarttime) && circuitState) 
      {
        digitalWrite(relayPin,HIGH);
      }
       //Otherwise the emptying solenoid has been open long enough and should close
      else
      {
        digitalWrite(relayPin,LOW);
        
        //Check pressure
        pressure = analogRead(analogPin);
        
        break;
      }
  }

  //Ensure the emptying solenoid is closed
  digitalWrite(relayPin,LOW);
}


//-------------------------------------------------------------------------------------------------------------
//Function used to purge, opens both solenoids for a set amount of time
//-------------------------------------------------------------------------------------------------------------
void Purge(bool& circuitState, int intakerelayPin, int exhaustrelayPin)
{
      String circuitName; 
      long int purgeTime = 0;
          
      // Check for current time and record it 
      unsigned long previousTime = millis();
      
      //Set relays to high opening the solenoids
      
      digitalWrite(intakerelayPin, HIGH);  
      digitalWrite(exhaustrelayPin, HIGH);  

      //Both solenoids are open
      bool relaysareOn = true;

      //Display to the user the current status
      switch(intakerelayPin)
      {
        case 23:
          circuitName = "MARX";
          purgeTime = marxPurgeTime;
          break;

        case 25:
          circuitName = "MTG";
          purgeTime = mtgPurgeTime;
          break;

        case 27:
          circuitName = "SWITCH";
          purgeTime = switchPurgeTime;
          break;

        case 29:
          circuitName = "SWITCH TG70";
          purgeTime = tg70switchPurgeTime;
          break;

        case 31:
          circuitName = "MARX TG70";
          purgeTime = tg70marxPurgeTime;
          break;
      } 

      // While relays are open continue checking how much time has passed
      while(relaysareOn && purgeState && circuitState)
      {
        //Check the state of the front panel buttons
        ControlButtonStateManager();

        //lcd.print("PURGING:        ");   //Will be used for LOG FUNCTION
        //lcd.print(String(circuitName + "                "));   //Will be used for LOG FUNCTION
        
        unsigned long currentTime = millis();

        // When the required time has passed turn off relays
        if(currentTime - previousTime >= purgeTime)
        {
          
          digitalWrite(intakerelayPin, LOW);  
          digitalWrite(exhaustrelayPin, LOW);  
          
          //Both solenoids are now closed
          relaysareOn = false;
        } 
      }
    //Ensure solenoids are now closed
    digitalWrite(intakerelayPin, LOW);  
    digitalWrite(exhaustrelayPin, LOW);  
}


//-------------------------------------------------------------------------------------------------------------
//Fucntion for monitoring the state of the front panel buttons
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateManager()
{ 
  //Check the supply pressure
  if(!errorState)
  {
    checkSupply();
  }

  //Run the automatic reclaimer check if needed
  if(automatereclaimerState && !errorState)
  {
    automaticReclaimerControl();
  }
  else if(!automatereclaimerState && !errorState)
  {
    //Run the manual reclaimer controller if needed
    manualReclaimerControl();    
  }
  
  //Check buttons for HIGH or LOW
  int readmarxenableState = digitalRead(marxenablePin);
  int readmtgenableState = digitalRead(mtgenablePin);
  int readswitchenableState = digitalRead(switchenablePin);
  int readtg70switchenableState = digitalRead(tg70switchenablePin);
  int readtg70marxenableState = digitalRead(tg70marxenablePin);
  int readautomatereclaimerState = digitalRead(automatereclaimerPin);
  int readpurgeState = digitalRead(purgePin);
  int readshotmodeState = digitalRead(shotmodePin);
  int readalarmState = digitalRead(alarmPin);
  int readstartreclaimerState = digitalRead(reclaimerstartenablePin);
  int readstopreclaimerState = digitalRead(reclaimerstopenablePin);
  int readabortState = digitalRead(abortbuttonPin);

  //Check the state of the buttons if they have been pressed
  ControlButtonStateCheck(readmarxenableState, marxenableState, lastmarxenableState);
  ControlButtonLEDStateCheck(marxenableState,marxenableLEDPin);
  
  ControlButtonStateCheck(readmtgenableState, mtgenableState, lastmtgenableState);
  ControlButtonLEDStateCheck(mtgenableState, mtgenableLEDPin);
  
  ControlButtonStateCheck(readswitchenableState, switchenableState, lastswitchenableState);
  ControlButtonLEDStateCheck(switchenableState, switchenableLEDPin);
  
  ControlButtonStateCheck(readtg70switchenableState, tg70switchenableState, lasttg70switchenableState);
  ControlButtonLEDStateCheck(tg70switchenableState, tg70switchenableLEDPin);
  
  ControlButtonStateCheck(readtg70marxenableState, tg70marxenableState, lasttg70marxenableState);
  ControlButtonLEDStateCheck(tg70marxenableState, tg70marxenableLEDPin);
  
  ControlButtonStateCheck(readabortState, abortState, lastabortState);
  ControlButtonLEDStateCheck(abortState, abortLEDPin);
  
  ControlButtonStateCheck(readpurgeState, purgeState, lastpurgeState);
  ControlButtonLEDStateCheck(purgeState, purgeLEDPin);
  
  ControlButtonStateCheck(readshotmodeState, shotmodeState, lastshotmodeState);
  ControlButtonLEDStateCheck(shotmodeState, shotmodeLEDPin);

  ControlButtonStateCheck(readautomatereclaimerState, automatereclaimerState, lastautomatereclaimerState);
  ControlButtonLEDStateCheck(automatereclaimerState, automatereclaimerLEDPin);
  
  ControlButtonStateCheckAlarm(readalarmState, alarmState, lastalarmState);

  ControlButtonStateCheckReclaimer(readstartreclaimerState, startreclaimerState, laststartreclaimerState);

  ControlButtonStateCheckReclaimer(readstopreclaimerState, stopreclaimerState, laststopreclaimerState);

  //If a state function is activated, set standbymode to true
  if((purgeState || shotmodeState || abortState ) && !errorState)
  {
    standbyMode = true;
  }
   
  //If the system is in manual mode, it should not run the program and disable these states.
  if(!automaticMode && !errorState)
  {
    automatereclaimerState = false;
    purgeState = false;
    shotmodeState = false;
    digitalWrite(automatereclaimerLEDPin, false);
    digitalWrite(purgeLEDPin, false);
    digitalWrite(shotmodeState, false);
  }

  //If the user aborts a shot, shotmode and purge should be disabled.
  if(abortState && !errorState)
  {
    purgeState = false;
    shotmodeState = false;
  }
}


//-------------------------------------------------------------------------------------------------------------
//Check the state of the buttons
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateCheck(int reading, bool& buttonState, bool& lastbuttonState)
{
  if(!errorState || (errorState && alarmState))
  {
    if(reading == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
      SaveCurrentSettings();
    }
    else if(reading == HIGH)
    {
      lastbuttonState = false;
    }
  }
}


//-------------------------------------------------------------------------------------------------------------
//Check the state of the reclaimer control buttons
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateCheckReclaimer(int reading, bool& buttonState, bool& lastbuttonState)
{
  if(!automatereclaimerState)
  {
    if(reading == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
      SaveCurrentSettings();
    }
    else if(reading == HIGH)
    {
      lastbuttonState = false;
    }
  }
}


//-------------------------------------------------------------------------------------------------------------
//Separate check for the state of the alarm button
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateCheckAlarm(int reading, bool& buttonState, bool& lastbuttonState)
{
  if(errorState)
  {
    if(reading == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
    }
    else if(reading == HIGH)
    {
      lastbuttonState = false;
    }
  }
}


//-------------------------------------------------------------------------------------------------------------
//Using the state of the button, set the LED on or off
//-------------------------------------------------------------------------------------------------------------
void ControlButtonLEDStateCheck(bool buttonState, const int ledPin)
{
    digitalWrite(ledPin, buttonState);
}


//-------------------------------------------------------------------------------------------------------------
//System log function
//-------------------------------------------------------------------------------------------------------------
void sysLog(unsigned long timeStamp, String logString)
{
  //Check is an SD card is present and active. If one is present, save the log data. If not skip to return.
  File logFile = SD.open("Log.txt", FILE_WRITE);  //Save the log string
  logFile.seek(logFile.size());
  logFile.print(String(timeStamp / (1000*60*60)) + ":" + String(timeStamp / (1000*60)) + ":" + String(timeStamp / 1000));
  logFile.println(String(": " + logString));
  logFile.close();
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Updates the current settings to the file that is used when starting the program
//-------------------------------------------------------------------------------------------------------------
void SaveCurrentSettings()
{
  //Check is an SD card is present and active. If one is present, save the data. If not skip to return.
  if(sdCard)
  {
    SD.remove("Setting.txt");
    File lastPresetFile = SD.open("Setting.txt", FILE_WRITE);  //Save the setting
    lastPresetFile.println(alarmEnable);
    lastPresetFile.println(Marxsetpoint);
    lastPresetFile.println(MTGsetpoint);
    lastPresetFile.println(Switchsetpoint);
    lastPresetFile.println(TG70Switchsetpoint);
    lastPresetFile.println(TG70Marxsetpoint);
    lastPresetFile.println(maxReclaimerPressure);
    lastPresetFile.println(minReclaimerPressure);
    lastPresetFile.println(marxenableState);
    lastPresetFile.println(mtgenableState);
    lastPresetFile.println(switchenableState);
    lastPresetFile.println(tg70switchenableState);
    lastPresetFile.println(tg70marxenableState);
    lastPresetFile.println(marxmaxTime);
    lastPresetFile.println(mtgmaxTime);
    lastPresetFile.println(switchmaxTime);
    lastPresetFile.println(tg70switchmaxTime);
    lastPresetFile.println(tg70marxmaxTime);
    lastPresetFile.println(marxDelay);
    lastPresetFile.println(mtgDelay);
    lastPresetFile.println(switchDelay);
    lastPresetFile.println(tg70marxDelay);
    lastPresetFile.println(tg70switchDelay);
    lastPresetFile.println(marxPurgeTime);
    lastPresetFile.println(mtgPurgeTime);
    lastPresetFile.println(switchPurgeTime);
    lastPresetFile.println(tg70switchPurgeTime);
    lastPresetFile.println(tg70marxPurgeTime);
    lastPresetFile.println(minBottlePressure);
    lastPresetFile.println(kp_Marx);
    lastPresetFile.println(ki_Marx);
    lastPresetFile.println(kd_Marx);
    lastPresetFile.println(kp_MTG);
    lastPresetFile.println(ki_MTG);
    lastPresetFile.println(kd_MTG);
    lastPresetFile.println(kp_Switch);
    lastPresetFile.println(ki_Switch);
    lastPresetFile.println(kd_Switch);
    lastPresetFile.println(kp_SwitchTG70);
    lastPresetFile.println(ki_SwitchTG70);
    lastPresetFile.println(kd_SwitchTG70);
    lastPresetFile.println(kp_MarxTG70);
    lastPresetFile.println(ki_MarxTG70);
    lastPresetFile.println(kd_MarxTG70);
    lastPresetFile.println(reclaimerSafetyTime);
    lastPresetFile.println(Marxcalibration);
    lastPresetFile.println(MTGcalibration);
    lastPresetFile.println(Switchcalibration);
    lastPresetFile.println(TG70Switchcalibration);
    lastPresetFile.println(TG70Marxcalibration);
    lastPresetFile.println(Reclaimcalibration);
    lastPresetFile.println(Minsupplycalibration);

    // save circuit calibrations
    //for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
    //  lastPresetFile.println(*(calibration_map[i]));
   //}
    lastPresetFile.close();
  }
  sysLog(millis(), "System settings updated.");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Save presets
//-------------------------------------------------------------------------------------------------------------
String FileWriter(int presetNumber)
{
  ControlButtonStateManager();
  
  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");


  //Save the selected preset
  if(sdCard)
  { 
    //Display progress bar
    myNex.writeNum("Confirm_Preset.Progress_Bar.aph", 127);
    myNex.writeStr("Confirm_Preset.t0.txt", String("Saving Preset " + String(presetNumber) + String("...")));
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 0);

    SD.remove(file);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 2);
    File presetFile = SD.open(file, FILE_WRITE);  //Save the file
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 4);
    presetFile.println(alarmEnable);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 6);
    presetFile.println(Marxsetpoint);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 8);
    presetFile.println(MTGsetpoint);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 10);
    presetFile.println(Switchsetpoint);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 12);
    presetFile.println(TG70Switchsetpoint);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 14);
    presetFile.println(TG70Marxsetpoint);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 16);
    presetFile.println(maxReclaimerPressure);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 18);
    presetFile.println(minReclaimerPressure);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 20);
    presetFile.println(marxenableState);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 22);
    presetFile.println(mtgenableState);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 24);
    presetFile.println(switchenableState);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 26);
    presetFile.println(tg70switchenableState);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 28);
    presetFile.println(tg70marxenableState);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 30);
    presetFile.println(marxmaxTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 32);
    presetFile.println(mtgmaxTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 34);
    presetFile.println(switchmaxTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 36);
    presetFile.println(tg70switchmaxTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 38);
    presetFile.println(tg70marxmaxTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 40);
    presetFile.println(marxDelay);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 42);
    presetFile.println(mtgDelay);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 44);
    presetFile.println(switchDelay);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 44);
    presetFile.println(tg70marxDelay);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 46);
    presetFile.println(tg70switchDelay);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 48);
    presetFile.println(marxPurgeTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 50);
    presetFile.println(mtgPurgeTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 52);
    presetFile.println(switchPurgeTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 54);
    presetFile.println(tg70switchPurgeTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 56);
    presetFile.println(tg70marxPurgeTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 58);
    presetFile.println(minBottlePressure);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 60);
    presetFile.println(kp_Marx);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 62);
    presetFile.println(ki_Marx);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 64);
    presetFile.println(kd_Marx);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 66);
    presetFile.println(kp_MTG);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 68);
    presetFile.println(ki_MTG);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 70);
    presetFile.println(kd_MTG);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 72);
    presetFile.println(kp_Switch);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 74);
    presetFile.println(ki_Switch);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 76);
    presetFile.println(kd_Switch);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 78);
    presetFile.println(kp_SwitchTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 80);
    presetFile.println(ki_SwitchTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 82);
    presetFile.println(kd_SwitchTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 84);
    presetFile.println(kp_MarxTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 86);
    presetFile.println(ki_MarxTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 88);
    presetFile.println(kd_MarxTG70);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 90);
    presetFile.println(reclaimerSafetyTime);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 91);
    presetFile.println(Marxcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 92);
    presetFile.println(MTGcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 93);
    presetFile.println(Switchcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 94);
    presetFile.println(TG70Switchcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 95);
    presetFile.println(TG70Marxcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 96);
    presetFile.println(Reclaimcalibration);
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 97);
    presetFile.println(Minsupplycalibration);
    presetFile.close();
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 98);
    SaveCurrentSettings();
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 99);

    //Checksum test goes here

    
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 100);
    sysLog(millis(), String("Preset " + String(presetNumber) + String(" saved!")));
    return String("Preset " + String(presetNumber) + String(" saved!"));  //Also log this
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    sysLog(millis(), "ERROR: SD card not found!");
    return "ERROR: SD card not found!";
  }
}


//-------------------------------------------------------------------------------------------------------------
//Load presets
//-------------------------------------------------------------------------------------------------------------
String FileReader(int presetNumber)
{
  ControlButtonStateManager();

  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");
  
  if(sdCard)
  {
    if(SD.exists(file)) //File is found.
    {

      //Display progress bar
      myNex.writeNum("Confirm_Preset.Progress_Bar.aph", 127);
      myNex.writeStr("Confirm_Preset.t0.txt", String("Loading Preset " + String(presetNumber) + String("...")));
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 0);
      
      File presetFile = SD.open(file, FILE_READ);
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 2);
      alarmEnable = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 4);
      Marxsetpoint = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 6);
      MTGsetpoint = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 8);
      Switchsetpoint = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 10);
      TG70Switchsetpoint = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 12);
      TG70Marxsetpoint = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 14);
      maxReclaimerPressure = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 16);
      minReclaimerPressure = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 18);
      marxenableState = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 20);
      mtgenableState = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 22);
      switchenableState = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 24);
      tg70switchenableState = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 26);
      tg70marxenableState = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 28);
      marxmaxTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 30);
      mtgmaxTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 32);
      switchmaxTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 34);
      tg70switchmaxTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 36);
      tg70marxmaxTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 38);
      marxDelay = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 40);
      mtgDelay = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 42);
      switchDelay = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 44);
      tg70marxDelay = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 46);
      tg70switchDelay = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 48);
      marxPurgeTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 50);
      mtgPurgeTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 52);
      switchPurgeTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 54);
      tg70switchPurgeTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 56);
      tg70marxPurgeTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 58);
      minBottlePressure = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 60);
      kp_Marx = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 62);
      ki_Marx = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 64);
      kd_Marx = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 66);
      kp_MTG = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 68);
      ki_MTG = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 70);
      kd_MTG = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 72);
      kp_Switch = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 74);
      ki_Switch = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 76);
      kd_Switch = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 78);
      kp_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 80);
      ki_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 82);
      kd_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 84);
      kp_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 86);
      ki_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 88);
      kd_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 90);
      reclaimerSafetyTime = presetFile.readStringUntil('\n').toInt();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 92);
      
      Marxcalibration = presetFile.readStringUntil('\n').toDouble();
      MTGcalibration = presetFile.readStringUntil('\n').toDouble();
      Switchcalibration = presetFile.readStringUntil('\n').toDouble();
      TG70Switchcalibration = presetFile.readStringUntil('\n').toDouble();
      TG70Marxcalibration = presetFile.readStringUntil('\n').toDouble();
      Reclaimcalibration = presetFile.readStringUntil('\n').toDouble();
      Minsupplycalibration = presetFile.readStringUntil('\n').toDouble();
      
      lastmarxenableState = !marxenableState;
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 93);
      lastmtgenableState = !mtgenableState;
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 94);
      lastswitchenableState = !switchenableState;
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 95);
      lasttg70switchenableState = !tg70switchenableState;
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 96);
      lasttg70marxenableState = !tg70marxenableState;
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 97);
      
      presetFile.close();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 98);
      SaveCurrentSettings();
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 99);

      //Checksum test goes here

      
      myNex.writeNum("Confirm_Preset.Progress_Bar.val", 100);
      sysLog(millis(), String("Preset " + String(presetNumber) + String(" loaded!")));
      return String("Preset " + String(presetNumber) + String(" loaded!"));  //Also log this
    } 
    else //No file is found, display error and return
    {
      myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
      sysLog(millis(), String(presetNumber) + String(" not found!"));
      return String("ERROR: Preset " + String(presetNumber) + String(" not found!"));
    }
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    sysLog(millis(), "ERROR: SD card not found!");
    return "ERROR: SD card not found!";
  }
}


//-------------------------------------------------------------------------------------------------------------
//Delete presets.
//-------------------------------------------------------------------------------------------------------------
String FileRemover(int presetNumber)
{
  ControlButtonStateManager();

  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");
  
  if(sdCard) //Check for SD card
  {
    if(SD.exists(file)) //File is found.
    {  
      SD.remove(file);
      sysLog(millis(), String("Preset " + String(presetNumber) + String(" deleted!")));
      return String("Preset " + String(presetNumber) + String(" deleted!"));
    }
    else //No file is found, display error and return
    {
      myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
      sysLog(millis(), String("ERROR: Preset " + String(presetNumber) + String(" not found!")));
      return String("ERROR: Preset " + String(presetNumber) + String(" not found!"));
    }
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    sysLog(millis(), "ERROR: SD card not found!");
    return "ERROR: SD card not found!";
  }
}


//-------------------------------------------------------------------------------------------------------------
//Set the pressure of a given circuit to the user specified value in PSI
//-------------------------------------------------------------------------------------------------------------
void SetCircuitPressure(int selection, float pressureValue)  
{
  double pressureSetpoint = 0;

  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Press");

  //pressureSetpoint = (pressureValue / 10) * (*calibration_map[selection]);

  //Set the new pressure setpoint to the correct circuit
  switch(selection)
  {
    case 0:
      pressureSetpoint = (pressureValue / 10) * Marxcalibration; //20.078
      Marxsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("Marx setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      sysLog(millis(), String("Marx setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;         
    case 1:
      pressureSetpoint = (pressureValue / 10) * TG70Marxcalibration; //20.089
      TG70Marxsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      sysLog(millis(), String("TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break; 
    case 2:
      pressureSetpoint = (pressureValue / 10) * MTGcalibration; //20
      MTGsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("MTG setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      sysLog(millis(), String("MTG setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;     
    case 3:
      pressureSetpoint = (pressureValue / 10) * Switchcalibration; //20.13
      Switchsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("Switch setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      sysLog(millis(), String("Switch setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;           
    case 4:
      // pressureSetpoint = (pressureValue / 10) * TG70Switchcalibration; //20.094
      TG70Switchsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("Switch TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      sysLog(millis(), String("Switch TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Press.t0.txt", "WARNING: Setting has failed!");
      sysLog(millis(), "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page Circuit_Select");
      return;
  }
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Circuit_Select");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Set the pressure of reclaimer to the user specified value in PSI
//-------------------------------------------------------------------------------------------------------------
void SetReclaimerPressure(int selection, float pressureValue)  
{ 
  double pressureSetpoint = 0;

  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Press");

  //pressureSetpoint = (pressureValue / 10) * (*calibration_map[(selection == 2) ? 6 : 5]);

  //Set the new pressure setpoint to the correct circuit
  switch(selection)
  {
    case 0: //Rec On
      pressureSetpoint = (pressureValue / 10) * Reclaimcalibration; //20
      //Sanity check
      if((pressureSetpoint - minReclaimerPressure <= 100) && (pressureSetpoint - minReclaimerPressure >= 0))
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window is too small!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;    
      }
      if(pressureSetpoint - minReclaimerPressure < 0)
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window cannot be negative!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return; 
      }
      else
      {
        maxReclaimerPressure = pressureSetpoint;
        myNex.writeStr("Confirm_Press.t0.txt", String("Reclaimer auto on set to " + String(pressureValue / 10) + String(" PSI.")));
        break;    
      }     

      
    case 1: //Rec Off
      if((maxReclaimerPressure - pressureSetpoint <= 100) && (pressureSetpoint - minReclaimerPressure >= 0))
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window is too small!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;     
      }
      if((maxReclaimerPressure - pressureSetpoint)< 0)
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window cannot be negative!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;  
      }
      else
      {
        pressureSetpoint = (pressureValue / 10) * Reclaimcalibration; //20
        minReclaimerPressure = pressureSetpoint;
        myNex.writeStr("Confirm_Press.t0.txt", String("Reclaimer auto off set to " + String(pressureValue / 10) + String(" PSI.")));
        break; 
      }
    case 2: //Supply
      pressureSetpoint = (pressureValue / 10) * Minsupplycalibration; //20
      minBottlePressure = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("Reclaimer min supply set to " + String(pressureValue / 10) + String(" PSI.")));
      break; 
          
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Press.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page Reclaimer");
      return;
  }
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Reclaimer");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Manually controls the reclaimer
//-------------------------------------------------------------------------------------------------------------
void manualReclaimerControl()
{
  //If the user assumes manual reclaimer control, then the states for the start and stop buttons must not be the same
  if(startreclaimerState && !automatereclaimerState)
  {
    if(!reclaimerRunning)
    {
      sysLog(millis(), "Reclaimer manually turned on.");
      stopreclaimerState = false;
      digitalWrite(reclaimerstartPin, HIGH);
      delay(100);
      digitalWrite(reclaimerstartPin, LOW);     
    }
    digitalWrite(startLEDPin, HIGH);
    digitalWrite(stopLEDPin, LOW);
    reclaimerRunning = true;
  } 
  if(stopreclaimerState && !automatereclaimerState)
  {
    if(reclaimerRunning)
    {
      sysLog(millis(), "Reclaimer manually turned off.");
      startreclaimerState = false;
      digitalWrite(reclaimerstopPin, HIGH);
      delay(100);
      digitalWrite(reclaimerstopPin, LOW);
    }
    digitalWrite(startLEDPin, LOW);
    digitalWrite(stopLEDPin, HIGH);
    reclaimerRunning = false;
  }
}


//-------------------------------------------------------------------------------------------------------------
//Automatically controls the reclaimer
//-------------------------------------------------------------------------------------------------------------
bool automaticReclaimerControl()
{
  //Set variables for pressure in the bottles
  double reclaimerPressure;
  reclaimerPressure = analogRead(reclaimeranaloginPin);
  unsigned long currentReclaimerTime = millis();

  //Turn on the reclaimer automatically if the pressure is too high and the safety time has passed
  if((!reclaimerRunning && (reclaimerPressure > maxReclaimerPressure) && automatereclaimerState))
  {
    if(currentReclaimerTime - previousReclaimerSafetyTime >= reclaimerSafetyTime)
    {
      delay(100);
      if(reclaimerPressure <= analogRead(reclaimeranaloginPin))
      {
        sysLog(millis(), "Reclaimer automoatically turned on.");
        reclaimerRunning = true;
        stopreclaimerState = false;
        startreclaimerState = true;
        digitalWrite(stopLEDPin, LOW);
        digitalWrite(reclaimerstartPin, HIGH);
        delay(100);
        digitalWrite(reclaimerstartPin, LOW);
      }
    }
  }

  //Turn off the reclaimer automatically if the pressure is too low
  if(reclaimerRunning && (reclaimerPressure < minReclaimerPressure) && automatereclaimerState)
  {
    sysLog(millis(), "Reclaimer automoatically turned off.");
    previousReclaimerSafetyTime = currentReclaimerTime;
    reclaimerRunning = false;
    stopreclaimerState = true;
    startreclaimerState = false;
    digitalWrite(startLEDPin, LOW);
    digitalWrite(reclaimerstopPin, HIGH);
    delay(100);
    digitalWrite(reclaimerstopPin, LOW);
  }

  //Blink corresponding LEDs
  unsigned long currentTimeLED = millis();
  if(reclaimerRunning && automatereclaimerState)
  {
    if(currentTimeLED - previousTime >= 500)
    {
      previousTime = currentTimeLED;
      if(digitalRead(startLEDPin) == LOW)
      {
        digitalWrite(startLEDPin, HIGH);
      }
      else
      {
        digitalWrite(startLEDPin, LOW);
      }
    }
  }
  if(!reclaimerRunning && automatereclaimerState)
  {
    if(currentTimeLED - previousTime >= 500)
    {
      previousTime = currentTimeLED;
      if(digitalRead(stopLEDPin) == LOW)
      {
        digitalWrite(stopLEDPin, HIGH);
      }
      else
      {
        digitalWrite(stopLEDPin, LOW);
      }
    }
  }
  return true;  
}

//-------------------------------------------------------------------------------------------------------------
//Monitor bottle supply pressure
//-------------------------------------------------------------------------------------------------------------
void checkSupply()
{
  double bottlePressure;
  bottlePressure = analogRead(bottleanaloginPin);
  
  //Check if the bottle pressure is too low, signal alarm.
  if(bottlePressure < minBottlePressure)
  {
   // errorState = true;
   // alarmController(String("LOW SUPPLY"));
  } 
}

//-------------------------------------------------------------------------------------------------------------
//Purge time setting.
//-------------------------------------------------------------------------------------------------------------
void purgeConfig(int selection, long int setTime)
{
  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Number");

  //Set the new purge time to the correct circuit
  switch(selection)
  {
    case 0:
      marxPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Marx purge time set to " + String(setTime) + String(" seconds.")));
      break;         
    case 1:
      tg70marxPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("TG70 purge time set to " + String(setTime) + String(" seconds.")));
      break; 
    case 2:
      mtgPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG purge time set to " + String(setTime) + String(" seconds.")));
      break;     
    case 3:
      switchPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch purge time set to " + String(setTime) + String(" seconds.")));
      break;           
    case 4:
      tg70switchPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch TG70 purge time set to " + String(setTime) + String(" seconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page Cir_Purge_Sel");
      return;
  }
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Cir_Purge_Sel");
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Circuit delay configuration function
//-------------------------------------------------------------------------------------------------------------
void circuitDelay(int selection, long int setTime)
{
  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Number");
  
  //Set the new circuit delay time to the correct circuit
  switch(selection)
  {
    case 0:
      marxDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("Marx delay set to " + String(setTime) + String(" milliseconds.")));
      break;         
    case 1:
      tg70marxDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      break; 
    case 2:
      mtgDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG delay set to " + String(setTime) + String(" milliseconds.")));
      break;     
    case 3:
      switchDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch delay set to " + String(setTime) + String(" milliseconds.")));
      break;           
    case 4:
      tg70switchDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page Cir_Delay_Sel");
      return;
  }

  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Cir_Delay_Sel");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Reclaimer safety delay setting function
//-------------------------------------------------------------------------------------------------------------
void setReclaimerSafetyDelay(long int setTime)
{
  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Number");

  //Set and save the new delay time
  reclaimerSafetyTime = setTime * 1000;
  myNex.writeStr("Confirm_Number.t0.txt", String("Reclaimer safety delay set to " + String(setTime) + String(" seconds.")));
  sysLog(millis(), String("Reclaimer safety delay set to " + String(setTime) + String(" seconds.")));
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Timers");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Set the selected PID variables for a given circuit
//-------------------------------------------------------------------------------------------------------------
void setPID(int selection, int tuneVariable, float setPID)
{
  String PIDvariableName;
  double kp = 0;
  double ki = 0;
  double kd = 0;
  double newPID = 0;

  ControlButtonStateManager();
  myNex.writeStr("page Confirm_PID");

  //Check which circuit we are setting the PID for, and retrieve the corresponidng value
  switch (selection)
  {
    case 0:
      kp = kp_Marx;
      ki = ki_Marx;
      kd = kd_Marx;
      break;
    
    case 1:
      kp = kp_MarxTG70;
      ki = ki_MarxTG70;
      kd = kd_MarxTG70;
      break;
      
    case 2:
      kp = kp_MTG;
      ki = ki_MTG;
      kd = kd_MTG;
      break;
      
    case 3:
      kp = kp_Switch;
      ki = ki_Switch;
      kd = kd_Switch;
      break;
            
    case 4:
      kp = kp_SwitchTG70;
      ki = ki_SwitchTG70;
      kd = kd_SwitchTG70;
      break;

    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_PID.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_PID.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page PID");
      return;
  }
  
  //Set the correct PID variable

  newPID = setPID / 10;
  
  switch (tuneVariable)
  {
    case 0:
      kp = newPID;
      PIDvariableName = " KP set to: ";
      break;
      
    case 1:
      ki = newPID;
      PIDvariableName = " KI set to: ";
      break;

    case 2:
      kd = newPID;
      PIDvariableName = " KD set to: ";
      break;
      
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_PID.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_PID.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page PID");
      return;
  }
  
    switch (selection)
  {
    case 0:
      kp_Marx = kp;
      ki_Marx = ki;
      kd_Marx = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("Marx PID" + PIDvariableName + String(newPID) + String(".")));
      break;
    
    case 1:
      kp_MarxTG70 = kp;
      ki_MarxTG70 = ki;
      kd_MarxTG70 = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("Marx TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      break;
      
    case 2:
      kp_MTG = kp;
      ki_MTG = ki;
      kd_MTG = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("MTG PID" + PIDvariableName + String(newPID) + String(".")));
      break;
      
    case 3:
      kp_Switch = kp;
      ki_Switch = ki;
      kd_Switch = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("Switch PID" + PIDvariableName + String(newPID) + String(".")));
      break;
            
    case 4:
      kp_SwitchTG70 = kp;
      ki_SwitchTG70 = ki;
      kd_SwitchTG70 = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("Switch TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      break;

    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_PID.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_PID.t0.txt", "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page PID");
      return;
  }
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page PID");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Alarm configuration function for circuit timeout
//-------------------------------------------------------------------------------------------------------------
void alarmConfig(int selection, long int setTime)
{
  ControlButtonStateManager();
  myNex.writeStr("page Confirm_Number");

  //Set the new alarm time to the correct circuit
  switch(selection)
  {
    case 0:
      marxmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Marx alarm time set to " + String(setTime) + String(" seconds.")));
      sysLog(millis(), String("Marx alarm time set to " + String(setTime) + String(" seconds.")));
      break;         
    case 1:
      tg70marxmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      sysLog(millis(), String("TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      break; 
    case 2:
      mtgmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG alarm time set to " + String(setTime) + String(" seconds.")));
      sysLog(millis(), String("MTG alarm time set to " + String(setTime) + String(" seconds.")));
      break;     
    case 3:
      switchmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch alarm time set to " + String(setTime) + String(" seconds.")));
      sysLog(millis(), String("Switch alarm time set to " + String(setTime) + String(" seconds.")));
      break;           
    case 4:
      tg70switchmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Switch TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      sysLog(millis(), String("Switch TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      sysLog(millis(), "WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page Cir_Alarm_Sel");
      return;
  }
  SaveCurrentSettings();
  delay(1500);
  myNex.writeStr("page Cir_Alarm_Sel");
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Alarm controller checks if condition for alarm is met
//-------------------------------------------------------------------------------------------------------------

void alarmController(String errorString)
{
  /*
  //Turn on the alarm, display error to user
  while(errorState && !alarmState)
  {
    ControlButtonStateManager();
    standbyMode = false;
    //Sound the alarm
    if(alarmEnable)
    {
      digitalWrite(alarmsoundPin, HIGH);
    }
    digitalWrite(alarmLEDPin, HIGH);
    //lcd.print("ALARM:          ");   //Will be used for LOG FUNCTION
    //lcd.print(errorString);  //Will be used for LOG FUNCTION
  }
  
  //User has pressed the alarm button. Turn off the alarm, display error to user, LED blinks
  while(errorState && alarmState)   
  {
    standbyMode = false;
      ControlButtonStateManager();
    digitalWrite(alarmsoundPin,LOW);  
    //lcd.print("CHECK ALARM:    ");   //Will be used for LOG FUNCTION
    //lcd.print(errorString);  //Will be used for LOG FUNCTION

    //Blink the button LEDs without delay
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(digitalRead(alarmLEDPin) == LOW)
      {
        digitalWrite(alarmLEDPin, HIGH);
      }
      else
      {
        digitalWrite(alarmLEDPin, LOW);
      }
    }
        
  }
  //User has cleared the error. The alarm is turned off if conditions are fine. If the error is not cleared, it will trigger again at the next check.
  if(errorState && !alarmState)
  {
    digitalWrite(alarmsoundPin,LOW);
    digitalWrite(alarmLEDPin, LOW);
    errorState = false;
    standbyMode = true;
    //lcd.print("ALARM CLEARED   ");   //Will be used for LOG FUNCTION
    //lcd.print(errorString);  //Will be used for LOG FUNCTION
    delay(3000);
  }
  */
}

//Read Presets

void readPresets() {
  if(SD.exists("Presets.txt")) 
  {
    File PresetsFile = SD.open("Presets.txt", FILE_READ);
    preset1 = PresetsFile.readStringUntil('\n');
    preset2 = PresetsFile.readStringUntil('\n');
    preset3 = PresetsFile.readStringUntil('\n');
    preset4 = PresetsFile.readStringUntil('\n');
    preset5 = PresetsFile.readStringUntil('\n');
    preset6 = PresetsFile.readStringUntil('\n');
    brightness = PresetsFile.readStringUntil('\n').toInt();
    color = PresetsFile.readStringUntil('\n').toInt();
    PresetsFile.close();
  }
  else if(sdCard && SD.begin(csPin)) {
    File PresetsFile = SD.open("Presets.txt", FILE_WRITE);
    preset1 = String("Preset 1");
    preset2 = String("Preset 2");
    preset3 = String("Preset 3");
    preset4 = String("Preset 4");
    preset5 = String("Preset 5");
    preset6 = String("Preset 6");
    brightness = 100;
    color = 65535;
    PresetsFile.close();
  }
  else
  {
    preset1 = "Preset 1";
    preset2 = "Preset 2";
    preset3 = "Preset 3";
    preset4 = "Preset 4";
    preset5 = "Preset 5";
    preset6 = "Preset 6";
    brightness = 100;
    color = 65535;
  }

  myNex.writeStr("Preset_Rename.t1.txt", preset1);
  myNex.writeStr("Preset_Rename.t2.txt", preset2);
  myNex.writeStr("Preset_Rename.t3.txt", preset3);
  myNex.writeStr("Preset_Rename.t4.txt", preset4);
  myNex.writeStr("Preset_Rename.t5.txt", preset5);
  myNex.writeStr("Preset_Rename.t6.txt", preset6);
  
  myNex.writeStr("dim=" + brightness);

  myNex.writeStr("Global.bco=" + color);
  
  myNex.writeStr("Main_Menu.bco=" + color);
  
  myNex.writeStr("Presets_Select.bco=" + color);
  
  myNex.writeStr("Presets_Menu.bco=" + color);

  myNex.writeStr("Timers.bco=" + color);
  
  myNex.writeStr("PID.bco=" + color);
  
  myNex.writeStr("Circuit_Select.bco=" + color);
  
  myNex.writeStr("Alarms.bco=" + color);

  myNex.writeStr("Reclaimer.bco=" + color);
  
  myNex.writeStr("On_Off.bco=" + color);
  
  myNex.writeStr("Enter_Numbers.bco=" + color);
  
  myNex.writeStr("Confirm_Presets.bco=" + color);
  
  myNex.writeStr("Confirm_Press.bco=" + color);
  
  myNex.writeStr("Confirm_On_Off.bco=" + color);

  myNex.writeStr("Confirm_Set.bco=" + color);
  
  myNex.writeStr("Boot_Page.bco=" + color);
  
  myNex.writeStr("Log_Page.bco=" + color);
  
  myNex.writeStr("Preset_Yes_No.bco=" + color);
  
  myNex.writeStr("Cir_Purge_Sel.bco=" + color);
\  
  myNex.writeStr("Confirm_Number.bco=" + color);
  
  myNex.writeStr("Cir_Delay_Sel.bco=" + color);
\  
  myNex.writeStr("Cir_Alarm_Sel.bco=" + color);
\  
  myNex.writeStr("Options.bco=" + color);
\  
  myNex.writeStr("Brightness.bco=" + color);
  
  myNex.writeStr("Color.bco=" + color);
]  
  myNex.writeStr("Preset_Rename.bco=" + color);
\}


//Write Presets

void writePresets() 
{
  if(SD.exists("Presets.txt")) 
  {
    preset1 = myNex.readStr("Preset_Rename.t1.txt");
    preset2 = myNex.readStr("Preset_Rename.t2.txt");
    preset3 = myNex.readStr("Preset_Rename.t3.txt");
    preset4 = myNex.readStr("Preset_Rename.t4.txt");
    preset5 = myNex.readStr("Preset_Rename.t5.txt");
    preset6 = myNex.readStr("Preset_Rename.t6.txt");
    brightness = myNex.readNumber("Brightness.n0.val");
    color = myNex.readNumber("Gobal.color.val");

    File PresetsFile = SD.open("Presets.txt", FILE_WRITE);
    PresetsFile.println(preset1);
    PresetsFile.println(preset2);
    PresetsFile.println(preset3);
    PresetsFile.println(preset4);
    PresetsFile.println(preset5);
    PresetsFile.println(preset6);
    PresetsFile.println(brightness);
    PresetsFile.println(color);
    PresetsFile.close();

  }
  else
  {
    //make page to say no sd card found
  }
}


//--------------------------------------------------------------------------------------------
//TRIGGERS
//--------------------------------------------------------------------------------------------

//PRESET TRIGGERS
//Trigger to Save, Load, or Delete a preset
void trigger1() 
{
  int presetNum = 0;
  String action = "";
  presetNum = myNex.readNumber("Global.preset.val");
  action = myNex.readStr("Global.presetAction.txt");
  myNex.writeStr("page Confirm_Preset");

  //Check if serial communication is successful, then map to function
  if(action == "ERROR" || presetNum == 777777)
  {
    myNex.writeStr("Confirm_Preset.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communications failure!");
    delay(1500);
    myNex.writeStr("page Presets_Menu");
    return;
  }
  else
  {
    //Map to the correct function
    if(action == "SAVE")
    {
      sysLog(millis(), "Entered SAVE PRESET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileWriter(presetNum));
    }
    else if(action == "LOAD")
    {
      sysLog(millis(), "Entered LOAD PRESET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileReader(presetNum));  
    }
    else if(action == "DELETE")
    {
      sysLog(millis(), "Entered DELETE PRSET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileRemover(presetNum)); 
    }
    else
    {
      myNex.writeStr("vis Confirm_Preset.Warning_Icon,1");
      myNex.writeStr("Confirm_Preset.t0.txt", "ERROR: Could not execute command!");
      sysLog(millis(), "ERROR: COuld not execute command!");
      delay(1500);
      myNex.writeStr("page Presets_Menu");
      return;
    }
    
    delay(1500);
    myNex.writeStr("page Presets_Menu");
    return;
  }
}

//SET PURGE TIMES
void trigger2() 
{
  int circuitNum = 0;
  long int timer = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  timer = myNex.readNumber("Enter_Numbers.number.val");

  //Check if serial communication is successful, then map to function
  if(circuitNum == 777777 || timer == 777777)
  {
    myNex.writeStr("page Confirm_Number");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Purge_Sel");
    return;
  }
  else
  {
    sysLog(millis(), "Entered purge config menu.");
    purgeConfig(circuitNum, timer);
  }
}

//SET CIRCUIT DELAY
void trigger3() 
{
  int circuitNum = 0;
  long int timer = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  timer = myNex.readNumber("Enter_Numbers.number.val");

  //Check if serial communication is successful, then map to function
  if(circuitNum == 777777 || timer == 777777)
  {
    myNex.writeStr("page Confirm_Number");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Delay_Sel");
    return;
  }
  else
  {
    sysLog(millis(), "Entered circuit delay config menu.");
    circuitDelay(circuitNum, timer);
  }
}

//SET ALARM TIMES
void trigger4() 
{
  int circuitNum = 0;
  long int timer = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  timer = myNex.readNumber("Enter_Numbers.number.val");

    //Check if serial communication is successful, then map to function
  if(circuitNum == 777777 || timer == 777777)
  {
    myNex.writeStr("page Confirm_Number");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Alarm_Sel");
    return;
  }
  else
  {
    sysLog(millis(), "Entered circuit alarm config menu.");
    alarmConfig(circuitNum, timer);
  }
}

//SET RECLAIMER SAFETY DELAY
void trigger5() 
{
  long int timer = 0;
  timer = myNex.readNumber("Enter_Numbers.number.val");

  //Check if serial communication is successful, then map to function
  if(timer == 777777)
  {
    myNex.writeStr("page Confirm_Number");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Timers");
    return;
  }
  else
  {
    sysLog(millis(), "Entered reclaimer safety delay menu.");
    setReclaimerSafetyDelay(timer);
  }
}

//Set Pressure
void trigger6()
{
  int circuitNum = 0;
  float pressureValue = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  pressureValue = myNex.readNumber("Enter_Float.float.val");

  //Check if serial communication is successful, then map to function
  if(pressureValue == 777777 || circuitNum == 777777)
  {
    myNex.writeStr("page Confirm_Press");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Circuit_Select");
    return;
  }
  else
  {
    sysLog(millis(), "Entered circuit pressure config menu.");
    SetCircuitPressure(circuitNum, pressureValue);
  }
}

//Set Reclaimer Pressure
void trigger7()
{
  int circuitNum = 0;
  float pressureValue = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  pressureValue = myNex.readNumber("Enter_Float.float.val");

  //Check if serial communication is successful, then map to function
  if(pressureValue == 777777 || circuitNum == 777777)
  {
    myNex.writeStr("page Confirm_Press");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Reclaimer");
    return;
  }
  else
  {
    sysLog(millis(), "Entered reclaimer pressure config menu.");
    SetReclaimerPressure(circuitNum, pressureValue);
  }
}

/* CIRCUIT CALIBRATION TRIGGERS */

// main calibration computation and setting
void trigger8() {
  int cidx;
  double pressure, prev;
  int ar;
  char sbuf[64];

  // read user-input current circuit PSI
  pressure = myNex.readNumber("Calibration.float.val");
  pressure /= 10;

  // get circuit selection from Circuit.val as a circuit-map index
  cidx = myNex.readNumber("Global.Circuit.val");

  if (pressure == 777777 || cidx == 777777) {
    myNex.writeStr("page Confirm_Press");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Calibration");
    return;
  }
  sysLog(millis(), "Entered calibration menu.");

  cidx = CIRC_IDX(cidx);
  
  // store previous calibration coefficient
  prev = *calibration_map[cidx];
  // read current pressure-analog value
  ar = analogRead(analog_map[cidx]);

    // page to calibration-status page
  myNex.writeStr("page Calibrated");

  // if input is non-zero
  if (pressure >= 0.1) {
    // calculate slope/calibration-coefficient
    *(calibration_map)[cidx] = ar;
    *(calibration_map)[cidx] /= pressure;
    // set successful status message
    sprintf(sbuf, "CALIBRATION SUCCESSFUL!");
    sysLog(millis(), String("Successfully calibrated " + String(circuit_map[cidx]) + " from " + String(prev) + " to " + String(*calibration_map[cidx])));
    myNex.writeStr("Calibrated.status.txt", sbuf);

  } else {
    // set failed status message
    sprintf(sbuf, "CALIBRATION FAILED. (divide by zero)");
    sysLog(millis(), String("Failed to set calibration for PSI of " + String(pressure)));
    myNex.writeStr("Calibrated.status.txt", sbuf);
  }

  // state calibration changes to Calibrated page
  myNex.writeStr("Calibrated.t0.txt", String("Previous Calibration > " + String(prev)));
  myNex.writeStr("Calibrated.t1.txt", String("Current Calibration > " + String(*calibration_map[cidx])) + String(cidx));
  SaveCurrentSettings();
}

// calibration circuit selection
void trigger9() {
  int cidx;

  // get circuit-index from current selection
  cidx = CIRC_IDX(myNex.readNumber("Global.Circuit.val"));

  // update calibration-circuit-name from circuit-string map
  myNex.writeStr("Calibration.Circuit_Disp.txt", circuit_map[cidx]);
}


//Alarm sound configuration
void trigger10()
{
  //User has selected to change the alarm sound on or off. Send command to screen
  myNex.writeStr("On_Off.t0.txt", "Alarm Sound");
  myNex.writeStr("On_Off.t1.txt", "Toggle alarm sound");
  myNex.writeNum("On_Off.sw1.val", alarmEnable);
  if(alarmEnable)
  {
    sysLog(millis(), "Alarm sound enabled.");
  }
  else
  {
    sysLog(millis(), "Alarm sound disabled.");
  }
}

void trigger11()
{
  alarmEnable = myNex.readNumber("On_Off.sw1.val");
  SaveCurrentSettings();
}

//PIG Config trigger
void trigger12()
{
  int circuitNum = 0;
  int tuneVariable = 0;
  float newPID = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");
  tuneVariable = myNex.readNumber("Global.pidVar.val");
  newPID = myNex.readNumber("Enter_Float.float.val");

  //Check if serial communication is successful, then map to function
  if(tuneVariable == 777777 || circuitNum == 777777 || newPID == 777777)
  {
    myNex.writeStr("page Confirm_PID");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    sysLog(millis(), "ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page PID");
    return;
  }
  else
  {
    sysLog(millis(), "Entered PID config menu.");
     setPID(selection, tuneVariable, newPID);
  }
}

//Event log display
void trigger13()
{
  ControlButtonStateManager();

  if(sdCard)
  {
    if(SD.exists("Log.txt")) //File is found.
    { 
      File logFile = SD.open("Log.txt", FILE_READ);
      while(logFile.position() != logFile.size())
      {
        myNex.writeStr("Log_Page.BootText.txt+", String(logFile.readStringUntil('\n')));
        myNex.writeStr("Log_Page.BootText.txt+", "\r\n"); 
      }
      logFile.close();
    }
    else //File not found
    {
      myNex.writeStr("Log_Page.BootText.txt+", "WARNING: Log file not found!");
    }
  }
  else //SD card is not found.
  {
    myNex.writeStr("Log_Page.BootText.txt+", "WARNING: SD Card not found!");
  }
}


//Brightness
/*
void trigger20() {
  brightness = myNex.readNumber("Global.brightness.val");
  myNex.writeStr("dim=" + brightness);
}
*/

//Change Preset Names
void trigger21() {
  writePresets();
}

void trigger22() {
  color = myNex.readNUmber("Global.color.val");

  myNex.writeStr("Global.bco=" + color);
  
  myNex.writeStr("Main_Menu.bco=" + color);
  
  myNex.writeStr("Presets_Select.bco=" + color);
  
  myNex.writeStr("Presets_Menu.bco=" + color);

  myNex.writeStr("Timers.bco=" + color);
  
  myNex.writeStr("PID.bco=" + color);
  
  myNex.writeStr("Circuit_Select.bco=" + color);
  
  myNex.writeStr("Alarms.bco=" + color);

  myNex.writeStr("Reclaimer.bco=" + color);
  
  myNex.writeStr("On_Off.bco=" + color);
  
  myNex.writeStr("Enter_Numbers.bco=" + color);
  
  myNex.writeStr("Confirm_Presets.bco=" + color);
  
  myNex.writeStr("Confirm_Press.bco=" + color);
  
  myNex.writeStr("Confirm_On_Off.bco=" + color);

  myNex.writeStr("Confirm_Set.bco=" + color);
  
  myNex.writeStr("Boot_Page.bco=" + color);
  
  myNex.writeStr("Log_Page.bco=" + color);
  
  myNex.writeStr("Preset_Yes_No.bco=" + color);
  
  myNex.writeStr("Cir_Purge_Sel.bco=" + color);
\  
  myNex.writeStr("Confirm_Number.bco=" + color);
  
  myNex.writeStr("Cir_Delay_Sel.bco=" + color);
\  
  myNex.writeStr("Cir_Alarm_Sel.bco=" + color);
\  
  myNex.writeStr("Options.bco=" + color);
\  
  myNex.writeStr("Brightness.bco=" + color);
  
  myNex.writeStr("Color.bco=" + color);
]  
  myNex.writeStr("Preset_Rename.bco=" + color);
  
  writePresets();
  
}