//-------------------------------------------------------------------------------------------------------------
//NTF Automatic circuit pressure controller
//Vladislav Petrov
//Sean Rolandelli
//Bradley Sullivan
//Last modification April 23, 2024
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

//Menu flag bool
bool menuFlag = true;
  
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
unsigned long checkMarxTime = 100;
unsigned long checkMTGTime = 100;
unsigned long checkSwitchTime = 100;
unsigned long checkTG70SwitchTime = 100;
unsigned long checkTG70MarxTime = 100;

//Purge time defaults
long int marxPurgeTime = 120000;
long int mtgPurgeTime = 50000;
long int switchPurgeTime = 90000;
long int tg70switchPurgeTime = 60000;
long int tg70marxPurgeTime = 45000;

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
  "R. SWITCH",
  "R. SWITCH TG70",
  "RECLAIMER",
  "SUPPLY"
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

double *pid_map[][3] = {
  {&kp_Marx, &ki_Marx, &kd_Marx},
  {&kp_MarxTG70, &ki_MarxTG70, &kd_MarxTG70},
  {&kp_MTG, &ki_MTG, &kd_MTG},
  {&kp_Switch, &ki_Switch, &kd_Switch},
  {&kp_SwitchTG70, &ki_SwitchTG70, &kd_SwitchTG70}
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
unsigned int color = 65535;

//Global log
String globalLog = "";

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

  //Start the initial boot screen
  myNex.writeStr("page Boot_Page");
  myNex.writeStr("bootText.txt", "INITIALIZING...\r\n");
  //sysLog("INITIALIZING.");
  myNex.writeNum("Progress_Bar.val", 0);

  //Unit test setup
  myNex.writeStr("bootText.txt+", "Setting up unit tests...\r\n");
  //sysLog("Setting up unit tests.");
  aunit::TestRunner::setPrinter(&Serial);  //I would like to set this to output onto the screen eventually
  aunit::TestRunner::setTimeout(10);
  myNex.writeNum("Progress_Bar.val", 10);
    
  //Set alarm pin to OUTPUT mode.
  pinMode(alarmsoundPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "Alarm pin set.\r\n");
  //sysLog("Alarm pin set.");
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
  ////sysLog("Solenoid pins set.");
  myNex.writeNum("Progress_Bar.val", 30);

  //Set SDcard pin to OUTPUT mode.
  pinMode(csPin, OUTPUT);
  myNex.writeStr("bootText.txt+", "SD pin set.\r\n");  //Need to write to log also
  //sysLog("SD pin set.");
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
  //sysLog("LED pins set.");
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
  //sysLog("Button pins set.");
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
  //sysLog("Button pullup resistors activated.");
  myNex.writeNum("Progress_Bar.val", 70);

  //Startup process. Load the last used settings. If none are found, create some defaults.
  //Check if SD card exists
  myNex.writeStr("bootText.txt+", "Checking SD Card...\r\n");
  //sysLog("Checking SD Card.");
  SD.begin();
  myNex.writeNum("Progress_Bar.val", 80);
  if(!SD.begin())//No sd card is found. Set the circuit pressure to whatever they happen to be at the time
  { 
    sdCard = false;
    myNex.writeStr("bootText.txt+", "WARNING: SD card not found!\r\n");
    //sysLog("WARNING: SD card not found!");
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
    //sysLog("Loading previous settings.");
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

      brightness = previousSettingFile.readStringUntil('\n').toInt();
      color = previousSettingFile.readStringUntil('\n').toInt();
      
      lastmarxenableState = !marxenableState;
      lastmtgenableState = !mtgenableState;
      lastswitchenableState = !switchenableState;
      lasttg70switchenableState = !tg70switchenableState;
      lasttg70marxenableState = !tg70marxenableState;
      previousSettingFile.close();
//      readPresets();

      SaveCurrentSettings();
      myNex.writeStr("bootText.txt+", "Previous settings loaded successfully!\r\n");
      //sysLog("Previous settings loaded successfully!");
      myNex.writeNum("Progress_Bar.val", 90);
      delay(1500);
    }
    else //No previous settings are found. Set the circuit pressure to whatever they happen to be at the time
    {
      myNex.writeStr("bootText.txt+", "No previous settings found. Using default settings...\r\n");
      //sysLog("No previous settings found. Using default settings.");
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
  //sysLog("Boot complete.");
  myNex.writeNum("Progress_Bar.val", 100);
  myNex.writeNum("Global.brightness.val", brightness);
  myNex.writeStr(String("dim=" + String(brightness)));
  myNex.writeNum("Global.color.val", color);
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

  if(menuFlag)
  {
    myNex.writeStr("page Main_Menu");
    menuFlag = false;
  }
 
  //Start shotmode pressure setting sequence
  if(shotmodeState && automaticMode)
  {
    ShotPressure(false);
  }
  
  //Start purge sequence for all enabled systems
  if(purgeState && automaticMode)
  {
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
      myNex.writeStr("page PurgeMode");
      //sysLog("Starting purge cycle.");
      
      if(tg70marxenableState && purgeState && tg70marxFlag)
      {
        //sysLog("Purging Marx TG70.");
        tg70marxFlag = false;
        Purge(tg70marxenableState, tg70marxinPin, tg70marxoutPin);
        //sysLog("Marx TG70 purge complete.");
      }
      else if(tg70switchenableState && purgeState && tg70switchFlag)
      {
        //sysLog("Purging Switch TG70.");
        tg70switchFlag = false;
        Purge(tg70switchenableState, tg70switchinPin, tg70switchoutPin);
        //sysLog("Switch TG70 purge complete.");
      }
      else if(mtgenableState && purgeState && MTGFlag)
      {
        //sysLog("Purging MTG.");
        MTGFlag = false;
        Purge(mtgenableState, mtginPin, mtgoutPin);
        //sysLog("MTG Purge complete.");
      }
      else if(marxenableState && purgeState && marxFlag)
      {
        //sysLog("Purging Marx.");
        marxFlag = false;
        Purge(marxenableState, marxinPin, marxoutPin);
        //sysLog("Marx purge complete.");
      }
      else if(switchenableState  && purgeState && switchFlag)
      {
        //sysLog("Purging switch.");
        switchFlag = false;
        Purge(switchenableState, switchinPin, switchoutPin);
        //sysLog("Switch purge complete.");
      }
      else
      {
        menuFlag = true;
        break;
      }
    }
    
    //Raise enabled cicuits to half pressure if shotmode state is false
    if(!shotmodeState) 
    {  
      ShotPressure(true);
    }

    //Display messege to user that the purge is completed
    //sysLog("Purge cycle complete.");
    purgeState = false;
    standbyMode = true;
  }
  
  //Start abort pressure setting sequence
  if(abortState && automaticMode)
  {
    //sysLog("Aborting shot.");
    abortShot();
    //sysLog("Shot aborted.");
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

  //Create input/output variables for PID functions
  double Marxinput,MTGinput,Switchinput,TG70Switchinput,TG70Marxinput;
  double Marxincreaseoutput,MTGincreaseoutput,Switchincreaseoutput,TG70Switchincreaseoutput,TG70Marxincreaseoutput;
  double Marxdecreaseoutput,MTGdecreaseoutput,Switchdecreaseoutput,TG70Switchdecreaseoutput,TG70Marxdecreaseoutput;

  double MarxsetpointPID = Marxsetpoint;
  double MTGsetpointPID = MTGsetpoint;
  double SwitchsetpointPID = Switchsetpoint;
  double TG70SwitchsetpointPID = TG70Switchsetpoint;
  double TG70MarxsetpointPID = TG70Marxsetpoint;

  if(half)
  {
    MarxsetpointPID = MarxsetpointPID / divisor;
    MTGsetpointPID = MTGsetpointPID / divisor;
    SwitchsetpointPID = SwitchsetpointPID / divisor;
    TG70SwitchsetpointPID = TG70SwitchsetpointPID / divisor;
    TG70MarxsetpointPID = TG70MarxsetpointPID / divisor;

    MarxHIGH = MarxsetpointPID + range;
    MarxLOW = MarxsetpointPID - range;
    MTGHIGH = MTGsetpointPID + range;
    MTGLOW = MTGsetpointPID - range;
    SwitchHIGH = SwitchsetpointPID + range;
    SwitchLOW = SwitchsetpointPID - range;
    TG70SwitchHIGH = TG70SwitchsetpointPID + range;
    TG70SwitchLOW = TG70SwitchsetpointPID - range;
    TG70MarxHIGH = TG70MarxsetpointPID + range;
    TG70MarxLOW = TG70MarxsetpointPID - range;
  }
  else
  {
    myNex.writeStr("page Shotmode");
  }

  //Create PID functions, one for pressure increase and one for pressure decrease
  PID MarxIncreasePID(&Marxinput, &Marxincreaseoutput, &MarxsetpointPID, kp_Marx, ki_Marx, kd_Marx, DIRECT);
  PID MarxDecreasePID(&Marxinput, &Marxdecreaseoutput, &MarxsetpointPID, kp_Marx, ki_Marx, kd_Marx, REVERSE); 
  PID MTGIncreasePID(&MTGinput, &MTGincreaseoutput, &MTGsetpointPID, kp_MTG, ki_MTG, kd_MTG, DIRECT);
  PID MTGDecreasePID(&MTGinput, &MTGdecreaseoutput, &MTGsetpointPID, kp_MTG, ki_MTG, kd_MTG, REVERSE); 
  PID SwitchIncreasePID(&Switchinput, &Switchincreaseoutput, &SwitchsetpointPID, kp_Switch, ki_Switch, kd_Switch, DIRECT);
  PID SwitchDecreasePID(&Switchinput, &Switchdecreaseoutput, &SwitchsetpointPID, kp_Switch, ki_Switch, kd_Switch, REVERSE); 
  PID TG70SwitchIncreasePID(&TG70Switchinput, &TG70Switchincreaseoutput, &TG70SwitchsetpointPID, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, DIRECT);
  PID TG70SwitchDecreasePID(&TG70Switchinput, &TG70Switchdecreaseoutput, &TG70SwitchsetpointPID, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, REVERSE); 
  PID TG70MarxIncreasePID(&TG70Marxinput, &TG70Marxincreaseoutput, &TG70MarxsetpointPID, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, DIRECT);
  PID TG70MarxDecreasePID(&TG70Marxinput, &TG70Marxdecreaseoutput, &TG70MarxsetpointPID, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, REVERSE); 
  
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
  const int WindowSize = 10000;
  
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

    menuFlag = false;
    bool flag = false;

    //Check the pressures 
    Marxcurrentpressure = analogRead(marxanaloginPin);                    
    MTGcurrentpressure = analogRead(mtganaloginPin);
    Switchcurrentpressure = analogRead(switchanaloginPin);
    TG70Switchcurrentpressure = analogRead(tg70switchanaloginPin);
    TG70Marxcurrentpressure = analogRead(tg70marxanaloginPin);
      
    //Check If pressures are too low, if so call RaisePressure for each system that is low and enabled.
    if (Marxcurrentpressure < MarxLOW && marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkMarxTime >= marxDelay)
      {
        //sysLog("Raising Marx pressure.");
        RaisePressure(marxenableState, Marxcurrentpressure, MarxsetpointPID, marxinPin, marxanaloginPin, WindowSize, Marxinput, Marxincreaseoutput, MarxIncreasePID);
        checkMarxTime = millis(); 
      }
    }
    if (MTGcurrentpressure < MTGLOW && mtgenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkMTGTime >= mtgDelay)
      {
        //sysLog("Raising MTG pressure.");
        RaisePressure(mtgenableState, MTGcurrentpressure, MTGsetpointPID, mtginPin, mtganaloginPin, WindowSize, MTGinput, MTGincreaseoutput, MTGIncreasePID);
        checkMTGTime = millis();
      }
    }
    if (Switchcurrentpressure < SwitchLOW && switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkSwitchTime >= switchDelay)
      {
        //sysLog("Raising Switch pressure.");
        RaisePressure(switchenableState, Switchcurrentpressure, SwitchsetpointPID, switchinPin, switchanaloginPin, WindowSize, Switchinput, Switchincreaseoutput, SwitchIncreasePID);
        checkSwitchTime = millis();
      }
    }
    if (TG70Switchcurrentpressure < TG70SwitchLOW && tg70switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkTG70SwitchTime >= tg70switchDelay)
      {
        //sysLog("Raising Switch TG70 pressure.");
        RaisePressure(tg70switchenableState, TG70Switchcurrentpressure, TG70SwitchsetpointPID, tg70switchinPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchincreaseoutput, TG70SwitchIncreasePID);
        checkTG70SwitchTime = millis();
      }
    }
    if (TG70Marxcurrentpressure < TG70MarxLOW && tg70marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkTG70MarxTime >= tg70marxDelay)
      {
        //sysLog("Raising Marx TG70 pressure.");
        RaisePressure(tg70marxenableState, TG70Marxcurrentpressure, TG70MarxsetpointPID, tg70marxinPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxincreaseoutput, TG70MarxIncreasePID);
        checkTG70MarxTime = millis();
      }
    }
  
    //Check if pressures are too high, if so call ReducePressure for each system that is high and enabled. 
    if (Marxcurrentpressure > MarxHIGH && marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkMarxTime >= marxDelay)
      {
        //sysLog("Lowering Marx pressure.");
        ReducePressure(marxenableState, Marxcurrentpressure, MarxsetpointPID, marxoutPin, marxanaloginPin, WindowSize, Marxinput, Marxdecreaseoutput, MarxDecreasePID);
        checkMarxTime = millis();
      }
    }
    if (MTGcurrentpressure > MTGHIGH && mtgenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkMTGTime >= mtgDelay)
      {
        //sysLog("Lowering MTG pressure.");
        ReducePressure(mtgenableState, MTGcurrentpressure, MTGsetpointPID, mtgoutPin, mtganaloginPin, WindowSize, MTGinput, MTGdecreaseoutput, MTGDecreasePID);
        checkMTGTime = millis();
      }
    }
    if (Switchcurrentpressure > SwitchHIGH && switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkSwitchTime >= switchDelay)
      {
        //sysLog("Lowering Switch pressure.");
        ReducePressure(switchenableState, Switchcurrentpressure, SwitchsetpointPID, switchoutPin, switchanaloginPin,WindowSize, Switchinput, Switchdecreaseoutput, SwitchDecreasePID);
        checkSwitchTime = millis();
      }
    }
    if (TG70Switchcurrentpressure > TG70SwitchHIGH && tg70switchenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkTG70SwitchTime >= tg70switchDelay)
      {
        //sysLog("Lowering Switch TG70 pressure.");
        ReducePressure(tg70switchenableState, TG70Switchcurrentpressure, TG70SwitchsetpointPID, tg70switchoutPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchdecreaseoutput, TG70SwitchDecreasePID);
        checkTG70SwitchTime = millis();
      }
    }
    if (TG70Marxcurrentpressure > TG70MarxHIGH && tg70marxenableState && ((shotmodeState && !purgeState) || (purgeState && half)))
    {
      flag = true;
      if(millis() - checkTG70MarxTime >= tg70marxDelay)
      {
        //sysLog("Lowering Marx TG70 pressure.");
        ReducePressure(tg70marxenableState, TG70Marxcurrentpressure, TG70MarxsetpointPID, tg70marxoutPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxdecreaseoutput, TG70MarxDecreasePID);
        checkTG70MarxTime = millis();
      }
    }
    if(half && !flag)
    {
      break;
    }
  }

  menuFlag = true;

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
  //Create setpoint range
  double range = 5.00;

  //Create division coeficiant for standby mode
  double divisor = 2.00;

 //Create small acceptable range around setpoints
  double Marxsetpointabort = Marxsetpoint/divisor; 
  double MTGsetpointabort = MTGsetpoint/divisor; 
  double Switchsetpointabort = Switchsetpoint/divisor; 
  double TG70Switchsetpointabort = TG70Switchsetpoint/divisor; 
  double TG70Marxsetpointabort = TG70Marxsetpoint/divisor; 
  
  double MarxHIGH = Marxsetpointabort + range; 
  double MTGHIGH = MTGsetpointabort + range; 
  double SwitchHIGH = Switchsetpointabort + range; 
  double TG70SwitchHIGH = TG70Switchsetpointabort + range; 
  double TG70MarxHIGH = TG70Marxsetpointabort + range; 
 
  //Create input/output variables for PID functions
  double Marxinput,MTGinput,Switchinput,TG70Switchinput,TG70Marxinput;
  double Marxdecreaseoutput,MTGdecreaseoutput,Switchdecreaseoutput,TG70Switchdecreaseoutput,TG70Marxdecreaseoutput;

  //Create PID functions
  PID MarxDecreasePID(&Marxinput, &Marxdecreaseoutput, &Marxsetpointabort, kp_Marx, ki_Marx, kd_Marx, REVERSE); 
  PID MTGDecreasePID(&MTGinput, &MTGdecreaseoutput, &MTGsetpointabort, kp_MTG, ki_MTG, kd_MTG, REVERSE); 
  PID SwitchDecreasePID(&Switchinput, &Switchdecreaseoutput, &Switchsetpointabort, kp_Switch, ki_Switch, kd_Switch, REVERSE); 
  PID TG70SwitchDecreasePID(&TG70Switchinput, &TG70Switchdecreaseoutput, &TG70Switchsetpointabort, kp_SwitchTG70, ki_SwitchTG70, kd_SwitchTG70, REVERSE); 
  PID TG70MarxDecreasePID(&TG70Marxinput, &TG70Marxdecreaseoutput, &TG70Marxsetpointabort, kp_MarxTG70, ki_MarxTG70, kd_MarxTG70, REVERSE); 

  //Set PIDs to automatic
  MarxDecreasePID.SetMode(AUTOMATIC);
  MTGDecreasePID.SetMode(AUTOMATIC);
  SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70SwitchDecreasePID.SetMode(AUTOMATIC);
  TG70MarxDecreasePID.SetMode(AUTOMATIC);
 
  //Set maximum solenoid open time(in Milliseconds)
  const int WindowSize = 10000;

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
        //sysLog("Lowering Marx pressure.");
        ReducePressure(marxenableState, Marxcurrentpressure, Marxsetpointabort, marxoutPin, marxanaloginPin, WindowSize, Marxinput, Marxdecreaseoutput, MarxDecreasePID);
        checkMarxTime = millis();
      }
    }
    else if (MTGcurrentpressure > MTGHIGH && mtgenableState)
    {
      if(millis() - checkMTGTime >= mtgDelay)
      {
        //sysLog("Lowering MTG pressure.");
        ReducePressure(mtgenableState, MTGcurrentpressure, MTGsetpointabort, mtgoutPin, mtganaloginPin, WindowSize, MTGinput, MTGdecreaseoutput, MTGDecreasePID);
        checkMTGTime = millis();
      }
    }
    else if (Switchcurrentpressure > SwitchHIGH && switchenableState)
    {
      if(millis() - checkSwitchTime >= switchDelay)
      {
        //sysLog("Lowering Switch pressure.");
        ReducePressure(switchenableState, Switchcurrentpressure, Switchsetpointabort, switchoutPin, switchanaloginPin,WindowSize, Switchinput, Switchdecreaseoutput, SwitchDecreasePID);
        checkSwitchTime = millis();
      }
    }
    else if (TG70Switchcurrentpressure > TG70SwitchHIGH && tg70switchenableState)
    {
      if(millis() - checkTG70SwitchTime >= tg70switchDelay)
      {
        //sysLog("Lowering Switch TG70 pressure.");
        ReducePressure(tg70switchenableState, TG70Switchcurrentpressure, TG70Switchsetpointabort, tg70switchoutPin, tg70switchanaloginPin, WindowSize, TG70Switchinput, TG70Switchdecreaseoutput, TG70SwitchDecreasePID);
        checkTG70SwitchTime = millis();
      }
    }  
    else if (TG70Marxcurrentpressure > TG70MarxHIGH && tg70marxenableState)
    {
      if(millis() - checkTG70MarxTime >= tg70marxDelay)
      {
        //sysLog("Lowering Marx TG70 pressure.");
        ReducePressure(tg70marxenableState, TG70Marxcurrentpressure, TG70Marxsetpointabort, tg70marxoutPin, tg70marxanaloginPin, WindowSize, TG70Marxinput, TG70Marxdecreaseoutput, TG70MarxDecreasePID);
        checkTG70MarxTime = millis();
      }
    }
    else
    {
      break;
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
void RaisePressure(bool& circuitState, double &currentpressure, double Threshold,const int relayPin, const int analogPin, const int WindowSize, double& input, double& output,  PID& System)
{
  String circuitName;
  int maxTime;
  int minTime = 10;

  //initialize pressure
  double pressure = currentpressure;
  double startPressure = pressure;

  //Start the window timer
  unsigned long windowstarttime;
  windowstarttime = millis();

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
      circuitName = "R. SWITCH TG70";
      maxTime = tg70switchmaxTime;
      break;
    case 31:
      circuitName = "MARX TG70";
      maxTime = tg70marxmaxTime;
      break;
  }  

  //Loop while the pressure is too low 
  while ((pressure < Threshold) && (shotmodeState || purgeState || abortState) && circuitState)
    {
      //Check the states of the front panel buttons
      ControlButtonStateManager();

      //Check pressure
      pressure = analogRead(analogPin);          
      
      //Read pressure for input to PID
      input = analogRead(analogPin);

      //Use PID to compute how long solenoid should open for
      System.Compute();

      //Read current time
      unsigned long now = millis();

      //Adjust time window if necessary
      if((now - windowstarttime + minTime > WindowSize))
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
          alarmController(String(circuitName + " CLOSED!"));
          break;
        }
        if(startPressure > analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " OPEN!"));
          break;
        }
      }      

      //If PID output is greater than current time minus the window time then the filling solenoid should be open 
      if((output > now - windowstarttime) && circuitState) 
      {
        digitalWrite(relayPin,HIGH);
        //sysLog(String(circuitName + " solenoid open."));
      }
      
      //Otherwise the filling solenoid has been open long enough and should close
      else
      {
        digitalWrite(relayPin,LOW);
        //sysLog(String(circuitName + " solenoid closed."));
        break;
      }    
    }

   //Ensure the filling solenoid is closed
  digitalWrite(relayPin,LOW);
}


//-------------------------------------------------------------------------------------------------------------
//Function for lowering the pressure using PID control
//-------------------------------------------------------------------------------------------------------------
void ReducePressure(bool& circuitState, double &currentpressure, double Threshold, const int relayPin, const int analogPin,const int WindowSize, double& input, double& output,  PID& System)
{     
  String circuitName;
  int maxTime;
  int minTime = 500;

  //Initialize the pressure
  double pressure = currentpressure;
  double startPressure = pressure;

  //Start the window timer
  unsigned long windowstarttime;
  windowstarttime = millis();

  //Read pressure for input to PID
  input = analogRead(analogPin);

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
      circuitName = "R. SWITCH TG70";
      maxTime = tg70switchmaxTime;
      break;
    case 30:
      circuitName = "MARX TG70";
      maxTime = tg70marxmaxTime;
      break;
  }   

  //Loop while pressure is too high
  while ((pressure > Threshold) && (shotmodeState || purgeState || abortState) && circuitState)
    {
      //Check the states of the front panel buttons
      ControlButtonStateManager();
         
      //Use PID to compute how long solenoid should open for
      System.Compute();

      //Read current time
      unsigned long now = millis();

      //Adjust time window if necessary
      if((now - windowstarttime + minTime > WindowSize))
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
          alarmController(String(circuitName + " CLOSED!"));
          break;
        }
        if(startPressure < analogRead(analogPin))
        {
          digitalWrite(relayPin,LOW);
          errorState = true;
          alarmController(String(circuitName + " OPEN!"));
          break;
        }
      }  

      //If PID output is greater than current time minus the window time then the emptying solenoid should be open 
      if((output > now - windowstarttime) && circuitState) 
      {
        digitalWrite(relayPin,HIGH);
        //sysLog(String(circuitName + " solenoid open."));
      }
       //Otherwise the emptying solenoid has been open long enough and should close
      else
      {
        digitalWrite(relayPin,LOW);
        //sysLog(String(circuitName + " solenoid closed."));
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
          circuitName = "R. SWITCH";
          purgeTime = switchPurgeTime;
          break;
        case 29:
          circuitName = "R. SWITCH TG70";
          purgeTime = tg70switchPurgeTime;
          break;
        case 31:
          circuitName = "MARX TG70";
          purgeTime = tg70marxPurgeTime;
          break;
      } 
      
      //Set relays to high opening the solenoids
      digitalWrite(intakerelayPin, HIGH);  
      digitalWrite(exhaustrelayPin, HIGH);
      //sysLog(String(circuitName + " solenoids open."));  

      //Both solenoids are open
      bool relaysareOn = true;

      // While relays are open continue checking how much time has passed
      while(relaysareOn && purgeState && circuitState)
      {
        //Check the state of the front panel buttons
        ControlButtonStateManager();

        unsigned long currentTime = millis();

        // When the required time has passed turn off relays
        if(currentTime - previousTime >= purgeTime)
        {
          
          digitalWrite(intakerelayPin, LOW);  
          digitalWrite(exhaustrelayPin, LOW);
          //sysLog(String(circuitName + " solenoids closed."));  
          
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
  //Read screen
  if(!shotmodeState && !purgeState)
  {
    myNex.NextionListen();
  }
  
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

  //Check the state of the buttons if they have been pressed
  ControlButtonStateCheck(marxenablePin, marxenableState, lastmarxenableState);
  ControlButtonLEDStateCheck(marxenableState,marxenableLEDPin);
  
  ControlButtonStateCheck(mtgenablePin, mtgenableState, lastmtgenableState);
  ControlButtonLEDStateCheck(mtgenableState, mtgenableLEDPin);
  
  ControlButtonStateCheck(switchenablePin, switchenableState, lastswitchenableState);
  ControlButtonLEDStateCheck(switchenableState, switchenableLEDPin);
  
  ControlButtonStateCheck(tg70switchenablePin, tg70switchenableState, lasttg70switchenableState);
  ControlButtonLEDStateCheck(tg70switchenableState, tg70switchenableLEDPin);
  
  ControlButtonStateCheck(tg70marxenablePin, tg70marxenableState, lasttg70marxenableState);
  ControlButtonLEDStateCheck(tg70marxenableState, tg70marxenableLEDPin);
  
  ControlButtonStateCheck(abortbuttonPin, abortState, lastabortState);
  ControlButtonLEDStateCheck(abortState, abortLEDPin);
  
  ControlButtonStateCheck(purgePin, purgeState, lastpurgeState);
  ControlButtonLEDStateCheck(purgeState, purgeLEDPin);
  
  ControlButtonStateCheck(shotmodePin, shotmodeState, lastshotmodeState);
  ControlButtonLEDStateCheck(shotmodeState, shotmodeLEDPin);

  ControlButtonStateCheck(automatereclaimerPin, automatereclaimerState, lastautomatereclaimerState);
  ControlButtonLEDStateCheck(automatereclaimerState, automatereclaimerLEDPin);
  
  ControlButtonStateCheckAlarm(alarmPin, alarmState, lastalarmState);

  ControlButtonStateCheckReclaimer(reclaimerstartenablePin, startreclaimerState, laststartreclaimerState);

  ControlButtonStateCheckReclaimer(reclaimerstopenablePin, stopreclaimerState, laststopreclaimerState);

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
void ControlButtonStateCheck(int pin, bool& buttonState, bool& lastbuttonState)
{
  if(!errorState || (errorState && alarmState))
  {
    double debounceTimer;
    if(digitalRead(pin) == LOW)
    {
      debounceTimer = millis();
    }
    while(millis() - debounceTimer <= 50)
    {
    }
    if(digitalRead(pin) == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
      SaveCurrentSettings();
    }
    else if(digitalRead(pin) == HIGH)
    {
      lastbuttonState = false;
    }
  }
}


//-------------------------------------------------------------------------------------------------------------
//Check the state of the reclaimer control buttons
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateCheckReclaimer(int pin, bool& buttonState, bool& lastbuttonState)
{
  if(!automatereclaimerState)
  {
    double debounceTimer;
    if(digitalRead(pin) == LOW)
    {
      debounceTimer = millis();
    }
    while(millis() - debounceTimer <= 50)
    {
    }
    if(digitalRead(pin) == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
      SaveCurrentSettings();
    }
    else if(digitalRead(pin) == HIGH)
    {
      lastbuttonState = false;
    }
  }
}


//-------------------------------------------------------------------------------------------------------------
//Separate check for the state of the alarm button
//-------------------------------------------------------------------------------------------------------------
void ControlButtonStateCheckAlarm(int pin, bool& buttonState, bool& lastbuttonState)
{
  if(errorState)
  {
    double debounceTimer;
    if(digitalRead(pin) == LOW)
    {
      debounceTimer = millis();
    }
    while(millis() - debounceTimer <= 50)
    {
    }
    if(digitalRead(pin) == LOW && !lastbuttonState)
    {
      buttonState = !buttonState;
      lastbuttonState = true;
    }
    else if(digitalRead(pin) == HIGH)
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
void sysLog(String logString)
{
  unsigned long timeStamp = millis();

  
  globalLog = String(globalLog + String(String(timeStamp / (1000*60*60)) + ":" + String((timeStamp / (1000*60)) % 60) + ":" + String((timeStamp / 1000) % 60) + String(": " + logString) + String("\r\n")));
  
  
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
    lastPresetFile.println(brightness);
    lastPresetFile.println(color);
    lastPresetFile.close();
  }
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
    presetFile.println(brightness);
    presetFile.println(color);
    presetFile.close();
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 98);
    SaveCurrentSettings();
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 99);

    //Checksum test goes here

    
    myNex.writeNum("Confirm_Preset.Progress_Bar.val", 100);
    //sysLog(String("Preset " + String(presetNumber) + String(" saved!")));
    return String("Preset " + String(presetNumber) + String(" saved!"));
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    //sysLog("ERROR: SD card not found!");
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

      brightness = presetFile.readStringUntil('\n').toInt();
      color = presetFile.readStringUntil('\n').toInt();
      
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
      
      myNex.writeNum("Global.brightness.val", brightness);
      myNex.writeStr(String("dim=" + String(brightness)));
      myNex.writeNum("Global.color.val", color);
      
      //sysLog(String("Preset " + String(presetNumber) + String(" loaded!")));
      return String("Preset " + String(presetNumber) + String(" loaded!"));
    } 
    else //No file is found, display error and return
    {
      myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
      //sysLog(String(presetNumber) + String(" not found!"));
      return String("ERROR: Preset " + String(presetNumber) + String(" not found!"));
    }
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    //sysLog("ERROR: SD card not found!");
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
      //sysLog(String("Preset " + String(presetNumber) + String(" deleted!")));
      return String("Preset " + String(presetNumber) + String(" deleted!"));
    }
    else //No file is found, display error and return
    {
      myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
      //sysLog(String("ERROR: Preset " + String(presetNumber) + String(" not found!")));
      return String("ERROR: Preset " + String(presetNumber) + String(" not found!"));
    }
  }
  else //SD card is not found.
  {
    myNex.writeNum("Confirm_Preset.Warning_Image.aph", 127);
    //sysLog("ERROR: SD card not found!");
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
      //sysLog(String("Marx setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;         
    case 1:
      pressureSetpoint = (pressureValue / 10) * TG70Marxcalibration; //20.089
      TG70Marxsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      //sysLog(String("TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break; 
    case 2:
      pressureSetpoint = (pressureValue / 10) * MTGcalibration; //20
      MTGsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("MTG setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      //sysLog(String("MTG setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;     
    case 3:
      pressureSetpoint = (pressureValue / 10) * Switchcalibration; //20.13
      Switchsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("R. Switch setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      //sysLog(String("Switch setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;           
    case 4:
      pressureSetpoint = (pressureValue / 10) * TG70Switchcalibration; //20.094
      TG70Switchsetpoint = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("R. Switch TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      //sysLog(String("Switch TG70 setpoint set to " + String(pressureValue / 10) + String(" PSI.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Press.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
        //sysLog("ERROR: Reclaimer window is too small!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;    
      }
      if(pressureSetpoint - minReclaimerPressure < 0)
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window cannot be negative!");
        //sysLog("ERROR: Reclaimer window cannot be negative!");
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
        //sysLog("ERROR: Reclaimer window is too small!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;     
      }
      if((maxReclaimerPressure - pressureSetpoint)< 0)
      {
        myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
        myNex.writeStr("Confirm_Press.t0.txt", "ERROR: Reclaimer window cannot be negative!");
        //sysLog("ERROR: Reclaimer window cannot be negative!");
        delay(1500);
        myNex.writeStr("page Reclaimer");
        return;  
      }
      else
      {
        pressureSetpoint = (pressureValue / 10) * Reclaimcalibration; //20
        minReclaimerPressure = pressureSetpoint;
        myNex.writeStr("Confirm_Press.t0.txt", String("Reclaimer auto off set to " + String(pressureValue / 10) + String(" PSI.")));
        //sysLog(String("Reclaimer auto off set to " + String(pressureValue / 10) + String(" PSI.")));
        break; 
      }
    case 2: //Supply
      pressureSetpoint = (pressureValue / 10) * Minsupplycalibration; //20
      minBottlePressure = pressureSetpoint;
      myNex.writeStr("Confirm_Press.t0.txt", String("Reclaimer min supply set to " + String(pressureValue / 10) + String(" PSI.")));
      //sysLog(String("Reclaimer min supply set to " + String(pressureValue / 10) + String(" PSI.")));
      break; 
          
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Press.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Press.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
      //sysLog("Reclaimer manually turned on.");
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
      //sysLog("Reclaimer manually turned off.");
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
        //sysLog("Reclaimer automoatically turned on.");
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
    //sysLog("Reclaimer automoatically turned off.");
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
   errorState = true;
   alarmController(String("LOW SUPPLY!"));
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
      //sysLog(String("Marx purge time set to " + String(setTime) + String(" seconds.")));
      break;         
    case 1:
      tg70marxPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("Marx TG70 purge time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("Marx TG70 purge time set to " + String(setTime) + String(" seconds.")));
      break; 
    case 2:
      mtgPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG purge time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("MTG purge time set to " + String(setTime) + String(" seconds.")));
      break;     
    case 3:
      switchPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch purge time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("Switch purge time set to " + String(setTime) + String(" seconds.")));
      break;           
    case 4:
      tg70switchPurgeTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch TG70 purge time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("Switch TG70 purge time set to " + String(setTime) + String(" seconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
      //sysLog(String("Marx delay set to " + String(setTime) + String(" milliseconds.")));
      break;         
    case 1:
      tg70marxDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("Marx TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      //sysLog(String("Marx TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      break; 
    case 2:
      mtgDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG delay set to " + String(setTime) + String(" milliseconds.")));
      //sysLog(String("MTG delay set to " + String(setTime) + String(" milliseconds.")));
      break;     
    case 3:
      switchDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch delay set to " + String(setTime) + String(" milliseconds.")));
      //sysLog(String("Switch delay set to " + String(setTime) + String(" milliseconds.")));
      break;           
    case 4:
      tg70switchDelay = setTime;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      //sysLog(String("Switch TG70 delay set to " + String(setTime) + String(" milliseconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
  //sysLog(String("Reclaimer safety delay set to " + String(setTime) + String(" seconds.")));
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
  double oldPID = 0;

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
      //sysLog("WARNING: Setting has failed!");
      delay(1500);
      myNex.writeStr("page PID");
      return;
  }
  
  //Set the correct PID variable

  newPID = setPID / 10;
  
  switch (tuneVariable)
  {
    case 0:
      oldPID = kp;
      kp = newPID;
      PIDvariableName = " KP set to: ";
      break;
      
    case 1:
      oldPID = ki;
      ki = newPID;
      PIDvariableName = " KI set to: ";
      break;

    case 2:
      oldPID = kd;
      kd = newPID;
      PIDvariableName = " KD set to: ";
      break;
      
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_PID.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_PID.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
      //sysLog(String("Marx PID" + PIDvariableName + String(newPID) + String(".")));
      break;
    
    case 1:
      kp_MarxTG70 = kp;
      ki_MarxTG70 = ki;
      kd_MarxTG70 = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("Marx TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      //sysLog(String("Marx TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      break;
      
    case 2:
      kp_MTG = kp;
      ki_MTG = ki;
      kd_MTG = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("MTG PID" + PIDvariableName + String(newPID) + String(".")));
      //sysLog(String("MTG PID" + PIDvariableName + String(newPID) + String(".")));
      break;
      
    case 3:
      kp_Switch = kp;
      ki_Switch = ki;
      kd_Switch = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("R. Switch PID" + PIDvariableName + String(newPID) + String(".")));
      //sysLog(String("Switch PID" + PIDvariableName + String(newPID) + String(".")));
      break;
            
    case 4:
      kp_SwitchTG70 = kp;
      ki_SwitchTG70 = ki;
      kd_SwitchTG70 = kd;
      myNex.writeStr("Confirm_PID.t0.txt", String("R. Switch TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      //sysLog(String("Switch TG70 PID" + PIDvariableName + String(newPID) + String(".")));
      break;

    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_PID.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_PID.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
      //sysLog(String("Marx alarm time set to " + String(setTime) + String(" seconds.")));
      break;         
    case 1:
      tg70marxmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      break; 
    case 2:
      mtgmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("MTG alarm time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("MTG alarm time set to " + String(setTime) + String(" seconds.")));
      break;     
    case 3:
      switchmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch alarm time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("Switch alarm time set to " + String(setTime) + String(" seconds.")));
      break;           
    case 4:
      tg70switchmaxTime = setTime * 1000;
      myNex.writeStr("Confirm_Number.t0.txt", String("R. Switch TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      //sysLog(String("Switch TG70 alarm time set to " + String(setTime) + String(" seconds.")));
      break;
    default:  //Error condition
      //Switch to error screen indicating that the setting has failed.
      myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
      myNex.writeStr("Confirm_Number.t0.txt", "WARNING: Setting has failed!");
      //sysLog("WARNING: Setting has failed!");
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
  //Turn on the alarm, display error to user
  //sysLog(String("ALARM: " + errorString));
  myNex.writeStr("page ErrorScreen");
  myNex.writeStr("t0.txt", String("ALARM: " + errorString));
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
  }
  
  //User has pressed the alarm button. Turn off the alarm, display error to user, LED blinks
  while(errorState && alarmState)   
  {
    standbyMode = false;
    ControlButtonStateManager();
    digitalWrite(alarmsoundPin,LOW);  

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

  //sysLog("Alarm acknowledged.");
  //User has cleared the error. The alarm is turned off if conditions are fine. If the error is not cleared, it will trigger again at the next check.
  if(!alarmState && errorState)
  {
    errorState = false;
    standbyMode = true;
    digitalWrite(alarmsoundPin,LOW);
    digitalWrite(alarmLEDPin, LOW);
    //sysLog("Alarm cleared.");
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
    //sysLog("ERROR: Serial communications failure!");
    delay(1500);
    myNex.writeStr("page Presets_Menu");
    return;
  }
  else
  {
    //Map to the correct function
    if(action == "SAVE")
    {
      //sysLog("Entered SAVE PRESET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileWriter(presetNum));
    }
    else if(action == "LOAD")
    {
      //sysLog("Entered LOAD PRESET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileReader(presetNum));  
    }
    else if(action == "DELETE")
    {
      //sysLog("Entered DELETE PRSET menu.");
      myNex.writeStr("Confirm_Preset.t0.txt", FileRemover(presetNum)); 
    }
    else
    {
      myNex.writeStr("vis Confirm_Preset.Warning_Icon,1");
      myNex.writeStr("Confirm_Preset.t0.txt", "ERROR: Could not execute command!");
      //sysLog("ERROR: COuld not execute command!");
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Purge_Sel");
    return;
  }
  else
  {
    //sysLog("Entered purge config menu.");
    purgeConfig(circuitNum, timer);
  }
}


//DISPLAY PREVIOUS SETTING (PURGE TIME)
void trigger22() 
{
  int circuitNum = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");

  //Set the old value to be visible on the screen
  switch(circuitNum)
  {
    case 0:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(marxPurgeTime / 1000)));
      break;         
    case 1:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70marxPurgeTime / 1000)));
      break; 
    case 2:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(mtgPurgeTime / 1000)));
      break;     
    case 3:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(switchPurgeTime / 1000)));
      break;           
    case 4:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70switchPurgeTime / 1000)));
      break;
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Delay_Sel");
    return;
  }
  else
  {
    //sysLog("Entered circuit delay config menu.");
    circuitDelay(circuitNum, timer);
  }
}


//SHOW OLD VALUE (CIRCUIT DELAY)
void trigger23() 
{
  int circuitNum = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");

  //Set the old value to be visible on the screen
  switch(circuitNum)
  {
    case 0:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(marxDelay)));
      break;         
    case 1:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70marxDelay)));
      break; 
    case 2:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(mtgDelay)));
      break;     
    case 3:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(switchDelay)));
      break;           
    case 4:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70switchDelay)));
      break;
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Cir_Alarm_Sel");
    return;
  }
  else
  {
    //sysLog("Entered circuit alarm config menu.");
    alarmConfig(circuitNum, timer);
  }
}


//SHOW OLD VALUE (ALARM TIMES)
void trigger24() 
{
  int circuitNum = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");

  //Set the old value to be visible on the screen
  switch(circuitNum)
  {
    case 0:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(marxmaxTime / 1000)));
      break;         
    case 1:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70marxmaxTime / 1000)));
      break; 
    case 2:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(mtgmaxTime / 1000)));
      break;     
    case 3:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(switchmaxTime / 1000)));
      break;           
    case 4:
      myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(tg70switchmaxTime / 1000)));
      break;
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Timers");
    return;
  }
  else
  {
    //sysLog("Entered reclaimer safety delay menu.");
    setReclaimerSafetyDelay(timer);
  }
}


//SHOW OLD VALUE (RECLAIMER SAFETY DELAY)
void trigger25() 
{
  myNex.writeStr("Enter_Numbers.oldValue.txt", String("Old value: " + String(reclaimerSafetyTime / 1000)));  
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Circuit_Select");
    return;
  }
  else
  {
    //sysLog("Entered circuit pressure config menu.");
    SetCircuitPressure(circuitNum, pressureValue);
  }
}


//SHOW OLD VALUE (CIRCUIT PRESSURE)
void trigger26() 
{
  int circuitNum = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");

  //Set the old value to be visible on the screen
  switch(circuitNum)
  {
    case 0:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(Marxsetpoint / Marxcalibration)));
      break;         
    case 1:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(TG70Marxsetpoint / TG70Marxcalibration)));
      break; 
    case 2:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(MTGsetpoint / MTGcalibration)));
      break;     
    case 3:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(Switchsetpoint / Switchcalibration)));
      break;           
    case 4:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(TG70Switchsetpoint / TG70Switchcalibration)));
      break;
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Reclaimer");
    return;
  }
  else
  {
    //sysLog("Entered reclaimer pressure config menu.");
    SetReclaimerPressure(circuitNum, pressureValue);
  }
}


//SHOW OLD VALUE (RECLAIMER AND SUPPLY PRESSURE)
void trigger27() 
{
  int circuitNum = 0;
  circuitNum = myNex.readNumber("Global.Circuit.val");

  //Set the old value to be visible on the screen
  switch(circuitNum)
  {
    case 0:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(maxReclaimerPressure / Reclaimcalibration)));
      break;         
    case 1:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(minReclaimerPressure / Reclaimcalibration)));
      break; 
    case 2:
      myNex.writeStr("Enter_Float.oldValue.txt", String("Old value: " + String(minBottlePressure / Minsupplycalibration)));
      break;     
  }         
}


// main calibration computation and setting
void trigger8() {
  int cidx;
  double pressure, prev;
  int ar;

  // read user-input current circuit PSI
  pressure = myNex.readNumber("Calibration.float.val");

  // get circuit selection from Circuit.val as a circuit-map index
  cidx = myNex.readNumber("Global.Circuit.val");

  if (pressure == 777777 || cidx == 777777) {
    myNex.writeStr("page Confirm_Press");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page Calibration");
    return;
  }
  //sysLog("Entered calibration menu.");

  pressure /= 10;

  cidx = CIRC_IDX(cidx);
  
  // store previous calibration coefficient
  prev = *calibration_map[cidx];
  // read current pressure-analog value
  ar = analogRead(analog_map[cidx]);

    // page to calibration-status page
  myNex.writeStr("page Set_Status");

  // if input is non-zero
  if (pressure >= 0.1) {
    // calculate slope/calibration-coefficient
    *(calibration_map)[cidx] = ar;
    *(calibration_map)[cidx] /= pressure;
    // set successful status message
    //sysLog(String("Successfully calibrated " + String(circuit_map[cidx]) + " from " + String(prev) + " to " + String(*calibration_map[cidx])));
    myNex.writeStr("Set_Status.status.txt", "CALIBRATION SUCCESSFUL!");
  } else {
    // set failed status message
    //sysLog(String("Failed to set calibration for PSI of " + String(pressure)));
    myNex.writeStr("Set_Status.status.txt", "CALIBRATION FAILED. (divide by zero)");
  }

  // state calibration changes to Set_Status page
  myNex.writeStr("Set_Status.t0.txt", String("Previous Calibration > " + String(prev)));
  myNex.writeStr("Set_Status.t1.txt", String("Current Calibration > " + String(*calibration_map[cidx])) + String(cidx));
  SaveCurrentSettings();

  delay(1500);
  myNex.writeStr("page Calibration");
}


void trigger9() {
  int cidx, m;

  m = myNex.readNumber("Global.mod.val");

  // get circuit-index from current selection
  cidx = MOD(myNex.readNumber("Global.Circuit.val"), m);

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
    //sysLog("Alarm sound enabled.");
  }
  else
  {
    //sysLog("Alarm sound disabled.");
  }
}

void trigger11()
{
  alarmEnable = myNex.readNumber("On_Off.sw1.val");
  SaveCurrentSettings();
}

//PID Config trigger
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
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page PID");
    return;
  }
  else
  {
    //sysLog("Entered PID config menu.");
    setPID(selection, tuneVariable, newPID);
  }
}

//Event log display
void trigger13()
{  
  myNex.writeStr("Log_Page.BootText.txt+", globalLog);
  myNex.writeStr("BootText.val_y=BootText.maxval_y");
}

// does the PID setting
void trigger14() {
  int cidx, m, p;
  double n;
  char s[32];

  // read which circuits are being considered
  m = myNex.readNumber("Global.mod.val");
  // get circuit selection
  cidx = MOD(myNex.readNumber("Global.Circuit.val"), m);
  // get PID-var selection (KP, KI, or KD)
  p = myNex.readNumber("Global.pidVar.val");
  // get user-input PID val
  n = myNex.readNumber("Set_PID.float.val");

  // test for any serial read-errors
  if (p == 777777 || m == 777777 || cidx == 777777 || n == 777777) {
    myNex.writeStr("page Confirm_Press");
    myNex.writeNum("Confirm_Number.Warning_Image.aph", 127);
    myNex.writeStr("Confirm_Number.t0.txt", "ERROR: Serial communication failure!");
    //sysLog("ERROR: Serial communication failure!");
    delay(1500);
    myNex.writeStr("page PID");
    return;
  }

  // move decimal place
  n /= 10;

  // page to parameter-set status page
  myNex.writeStr("page Set_Status");

  // configure status messages
  sprintf(s, "%s", (p == 0) ? "KP" : ((p == 1) ? "KI" : (p == 2) ? "KD" : "N/A"));
  myNex.writeStr("Set_Status.status.txt", String("Successfully set ") + String(s) + String(" !"));

  myNex.writeStr("Set_Status.t0.txt", String("Previous ") + String(s) + " > " + String(*pid_map[cidx][p]));
  *pid_map[cidx][p] = n;
  myNex.writeStr("Set_Status.t1.txt", String("Current ")  + String(s) + " > " + String(*pid_map[cidx][p]));

  // save changes
  SaveCurrentSettings();

  // display delay and return to PID-set page
  delay(1500);
  myNex.writeStr("page Set_PID");
}

// pull current PID value
void trigger15() {
  int cidx, m, p;
  double cur;
  char s[32];

  // pull circuit modulus
  m = myNex.readNumber("Global.mod.val");
  // get circuit-index from current selection
  cidx = MOD(myNex.readNumber("Global.Circuit.val"), m);

  // read current PID-var
  p = myNex.readNumber("Global.pidVar.val");

  // update PID-var set-strings
  sprintf(s, "%s", (p == 0) ? "KP" : ((p == 1) ? "KI" : ((p == 2) ? "KD" : "N/A")));
  myNex.writeStr("Set_PID.t0.txt", String("Current ") + String(s));
  myNex.writeStr("Set_PID.t3.txt", String("New ") + String(s));

  // pull current PID-var config
  cur = *pid_map[cidx][p];

  myNex.writeStr("Set_PID.Circuit_Disp.txt", circuit_map[cidx]);

  // set current PID-var string
  myNex.writeStr("Set_PID.t4.txt", String(cur));
}

//Alarm
void trigger16()
{
    standbyMode = false;
    ControlButtonStateManager();
    digitalWrite(alarmsoundPin,LOW);  
    
    myNex.writeStr("Global.pagenum.val=dp");
}



//Color
void trigger18() {
  color = myNex.readNumber("Global.color.val");
  SaveCurrentSettings();
}


//Brightness
void trigger19() {
  myNex.writeNum("Brightness.h0.val", brightness);
  myNex.writeNum("Brightness.n0.val", brightness);
}

void trigger20() {
  brightness = myNex.readNumber("Global.brightness.val");
  SaveCurrentSettings();
}

//Change Preset Names
void trigger21() {
//  writePresets();
}
