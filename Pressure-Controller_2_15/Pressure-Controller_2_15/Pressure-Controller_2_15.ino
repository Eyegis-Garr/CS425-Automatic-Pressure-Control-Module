//-------------------------------------------------------------------------------------------------------------
//NTF Automatic circuit pressure controller
//Vladislav Petrov
//Last modification Febuary 15, 2023
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//Global Variables and Libraries
//-------------------------------------------------------------------------------------------------------------
#include <SD.h>
#include <SPI.h>
#include <Key.h>
#include <Keypad.h>
#include <PID_v1.h>
#include <LiquidCrystal.h>

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

  //Set pins for sending analog data from pressure sensors to displays, these are global
  //CURENTLY NOT IN USE, DISPLAYS ARE CONNECTED DIRECTLY TO SENSORS
  const int marxanalogoutPin = 7;
  const int mtganalogoutPin = 8;
  const int switchanalogoutPin = 9;
  const int tg70switchanalogoutPin = 10;
  const int tg70marxanalogoutPin = 11;
  const int reclaimeranalognoutPin = 12;
  const int bottleanalogoutPin = 13;

  //Set pins for control buttons, these are global
  const int shotmodePin = 34;
  const int purgePin = 35;
  const int alarmPin = 36;
  const int automatereclaimerPin = 37;
  const int abortbuttonPin = 1;

  //Set pins for enabling/disabling each system
  const int marxenablePin = 17;
  const int mtgenablePin = 18;
  const int switchenablePin = 19;
  const int tg70switchenablePin = 20;
  const int tg70marxenablePin = 21;

  //Set pins for automatic reclaimer control
  const int reclaimerstopenablePin = 14;
  const int reclaimerstartenablePin = 15;

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

  //Set pins for MENU control buttons, these are global
  const byte ROWS = 1;
  const byte COLS = 5;
  char keys[ROWS][COLS] = {
                           {'1', '2', '3', '4', '5'}
                          };
  byte rowPins[ROWS] = {2};
  byte colPins[COLS] = {3, 4, 5, 6, 7};
  Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

  //Set pins for MENU LCD, these are global
  const int rs = 8, en = 9, d4 = 10, d5 = 11, d6 = 12, d7 = 13;
  LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
  unsigned long lcdResetTime = 0;

  //Menu Info. These are global
  int currentMenu = 0;
  int lastSelection = 0;
  int selection = 0;
  int circuitSelection = 0;

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
 double Marxsetpoint,MTGsetpoint,Switchsetpoint,TG70Switchsetpoint,TG70Marxsetpoint = 0.00;
 double maxReclaimerPressure = 500.0;
 double minReclaimerPressure = 50.0;

 //Unused
 bool isCouple;

//-------------------------------------------------------------------------------------------------------------
//Setup
//-------------------------------------------------------------------------------------------------------------
void setup() 
{
  Serial.begin(9600); //This is just for debugging

    lcd.begin(16, 2);
    lcd.setCursor(0,0);
    lcd.print("  INITIALIZING  ");
    

    //Set alarm pin to OUTPUT mode.
    pinMode(alarmsoundPin, OUTPUT);

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

    //Set SDcard pin to OUTPUT mode.
    pinMode(csPin, OUTPUT);

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

  //Startup process. Load the last used settings. If none are found, create some defaults.
  //Check if SD card exists
  if(!SD.begin(csPin))//No sd card is found. Set the circuit pressure to whatever they happen to be at the time
  { 
    sdCard = false;
    lcd.setCursor(0,0);
    lcd.print("WARNING:        ");
    lcd.setCursor(0,1);
    lcd.print("SDCARD NOT FOUND");
    Marxsetpoint = analogRead(marxanaloginPin);                    
    MTGsetpoint = analogRead(mtganaloginPin);
    Switchsetpoint = analogRead(switchanaloginPin);
    TG70Switchsetpoint = analogRead(tg70switchanaloginPin);
    TG70Marxsetpoint = analogRead(tg70marxanaloginPin);
    delay(3000);
  }
  else
  {
    lcd.setCursor(0,0);
    lcd.print("LOADING PREVIOUS");
    lcd.setCursor(0,1);
    lcd.print("SETTINGS        ");
    delay(3000);
    sdCard = true;
    if(SD.exists("Setting.txt")) //Previous settings are found. Load the previous settings into the controller.
    {
      File previousSettingFile = SD.open("Setting.txt", FILE_READ);
      alarmEnable = previousSettingFile.readStringUntil('\n').toInt();
      isCouple = previousSettingFile.readStringUntil('\n').toInt();
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
//      reclaimerSafetyTime = previousSettingFile.readStringUntil('\n').toInt();
      lastmarxenableState = !marxenableState;
      lastmtgenableState = !mtgenableState;
      lastswitchenableState = !switchenableState;
      lasttg70switchenableState = !tg70switchenableState;
      lasttg70marxenableState = !tg70marxenableState;
      previousSettingFile.close();

      SaveCurrentSettings();

      lcd.setCursor(0,0);
      lcd.print("PREVIOUS SETTING");
      lcd.setCursor(0,1);
      lcd.print("LOAD SUCCESS    ");
      delay(3000);
    }
    else //No previous settings are found. Set the circuit pressure to whatever they happen to be at the time
    {
      lcd.setCursor(0,0);
      lcd.print("NO PREVIOUS SET.");
      lcd.setCursor(0,1);
      lcd.print("CREATING DEFAULT");
      Marxsetpoint = analogRead(marxanaloginPin);                    
      MTGsetpoint = analogRead(mtganaloginPin);
      Switchsetpoint = analogRead(switchanaloginPin);
      TG70Switchsetpoint = analogRead(tg70switchanaloginPin);
      TG70Marxsetpoint = analogRead(tg70marxanaloginPin);
      SaveCurrentSettings();
      delay(3000);
    } 
  }
}

//-------------------------------------------------------------------------------------------------------------
//Main loop
//-------------------------------------------------------------------------------------------------------------
void loop() 
{

  //Check the state of the buttons. This allows a user to press buttons at almost any time. You will see this function call everywhere.
  ControlButtonStateManager();

  //Check if a user has pressed a key on the keypad. If they have disable standby mode
  if(keypad.getKey())
  {
    standbyMode = false;
  }
  
  //If the user has pressed a key on the keypad, exit standby mode and take them to the menu
  while(!standbyMode)
  {
    ControlButtonStateManager();
    menu();
  }
  //Display a messege to the user indicating that the system is in standby mode or manual mode
  if(!automaticMode)
  {
    lcd.setCursor(0,0);
    lcd.print("  MANUAL MODE   ");
    lcd.setCursor(0,1);
    lcd.print("                ");
  }
  else
  {
    lcd.setCursor(0,0);
    lcd.print("    STANDBY     ");
    lcd.setCursor(0,1);
    lcd.print("                ");
  }

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
      lcd.setCursor(0,0);
      lcd.print("ENABLE CIRCUITS ");
      lcd.setCursor(0,1);
      lcd.print("                ");
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
      lcd.begin(16, 2);
      lcd.setCursor(0,0);
      lcd.print("PURGE COMPLETE  ");
      lcd.setCursor(0,1);
      lcd.print("                ");
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
    lcd.setCursor(0,0);
    lcd.print("ENABLE CIRCUITS ");
    lcd.setCursor(0,1);
    lcd.print("                ");
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
      lcd.setCursor(0,0);
      lcd.print("CIRCUITS AT     ");
      lcd.setCursor(0,1);
      lcd.print("SET PRESSURE    ");
    }
    //In standby mode, break the loop when pressure is set
    else if( half && ((millis() - checkMarxTime >= marxDelay * 2) && (millis() - checkMTGTime >= mtgDelay * 2) && (millis() - checkSwitchTime >= switchDelay * 2) && (millis() - checkTG70SwitchTime >= tg70switchDelay * 2) && (millis() - checkTG70MarxTime >= tg70marxDelay * 2)))
    {
      ControlButtonStateManager();
      lcd.setCursor(0,0);
      lcd.print("CIRCUITS AT     ");
      lcd.setCursor(0,1);
      lcd.print("HALF PRESSURE   ");
      delay(3000);
      break;
    }
    //Still setting pressure
    else
    {
      ControlButtonStateManager();
      lcd.setCursor(0,0);
      lcd.print("SETTING PRESSURE");
      lcd.setCursor(0,1);
      lcd.print("                ");
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
  lcd.setCursor(0,0);
  lcd.print("ABORTING SHOT   ");
  lcd.setCursor(0,1);
  lcd.print("                ");
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
      lcd.setCursor(0,0);
      lcd.print("CIRCUITS AT     ");
      lcd.setCursor(0,1);
      lcd.print("HALF PRESSURE   ");
      delay(3000);
      break;
    }
    //Still setting pressure
    else
    {
      ControlButtonStateManager();
      lcd.setCursor(0,0);
      lcd.print("SETTING TO      ");
      lcd.setCursor(0,1);
      lcd.print("HALF PRESSURE   ");
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
      lcd.setCursor(0,0);
      if(half)
      {
        lcd.print("RAISING TO HALF:");
      }
      else
      {
        lcd.print("RAISING:        ");            
      }
      lcd.setCursor(0,1);
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
      lcd.print(String(circuitName + "                "));
      
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
      lcd.setCursor(0,0);
      if(half)
      {
        lcd.print("LOWERING TO HALF:");
      }
      else
      {
        lcd.print("LOWERING:       ");            
      }
      lcd.setCursor(0,1);
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
      lcd.print(String(circuitName + "                "));    

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

        lcd.setCursor(0,0);
        lcd.print("PURGING:        ");
        lcd.setCursor(0,1);
        lcd.print(String(circuitName + "                "));
        
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
  //Reset LCD if necessary
  resetLCD();
  
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
    currentMenu = 0;
    selection = 0;
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
//LCD Menu Function. Gets user input from keypad and displays on LCD
//-------------------------------------------------------------------------------------------------------------
void menu()
{ 
  
    ControlButtonStateManager();
  
  //Setup the menus in a 2D array. First dimension lists the number of menus. Second dimension lists the number of options.
  String menu[9][10] = {
                      {"CONTROL MODE", "PRESETS", "SET PRESSURES", "SET TIMES", "ALARM CONFIG", "PID CONFIG", "EXIT", "                "}, 
                      {"SET PURGE TIMES", "SET CIRC. DELAY", "SET SAFETY DEL.", "EXIT", "                "}, 
                      {"SAVE PRESET", "LOAD PRESET", "DELETE PRESET", "EXIT", "                "}, 
                      {"SET MARX", "SET MARX TG70", "SET MTG", "SET SWITCH", "SET SWITCH TG70", "SET RECL. ON", "SET RECL. OFF", "SET MIN SUPPLY", "EXIT", "                "},
                      {"SOUND ON/OFF", "MARX ALARM", "MARX TG70 ALARM", "MTG ALARM", "SWITCH ALARM", "SWITCH TG70 ALARM", "EXIT", "                "},
                      {"MARX DELAY", "MARX TG70 DELAY", "MTG DELAY", "SWITCH DELAY", "SWITCH TG70 DELAY", "EXIT", "                "},
                      {"MARX PID", "MARX TG70 PID", "MTG PID", "SWITCH PID", "SWITCH TG70 PID", "EXIT", "                "},
                      {"KP", "KI", "KD", "EXIT"},
                      {"MARX PURGE", "MARX TG70 PURGE", "MTG PURGE", "SWITCH PURGE", "SWITCH TG70 PURGE", "EXIT", "                "}
                      };

  //The limits of options in each menu. Used for a counter in navigation.
  int limits[9] = {7, 4, 4, 9, 7, 6, 6, 4, 6};

  //A map of menus. Each element corresponds to the menu item that is mapped to the selection.
  int menuMap[9][9] = {
                       {NULL, 2, 3, 1, 4, 6, 0},
                       {8, 5, NULL, 0},
                       {NULL, NULL, NULL, 0},
                       {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0},
                       {NULL, NULL, NULL, NULL, NULL, NULL, 0},
                       {NULL, NULL, NULL, NULL, NULL, 1},
                       {7, 7, 7, 7, 7, 0},
                       {NULL, NULL, NULL, 6},
                       {NULL, NULL, NULL, NULL, NULL, 1}
                       };


  //Get user input
  char key = keypad.getKey();
  if(key)
  {
    switch(key)
    {
      case '1': //Select
        if(currentMenu >= 8)  //If we scroll to the last menu item, do not increment up
        {
          currentMenu = 8;
        }
        if(currentMenu <= 0 || currentMenu == NULL)  //If we scroll to the last menu item, do not increment down
        {
          currentMenu = 0;
        }
        if((currentMenu == 1) && (selection == 2)) //Select to adjust the reclaimer safety delay
        {
          setReclaimerSafetyDelay();
          break;
        }
        if((currentMenu == 4) && (selection == 0)) //Selected turn alarm on or off
        {
          setAlarmOnOff();
          break;
        }
        if((currentMenu == 4) && (selection != 0) && (selection != 6)) //Select to set alarm tme of a circuit
        {
          alarmConfig(selection);
          break;
        }
        if((currentMenu == 5) && (selection != 5)) //Select to set delay time of a circuit
        {
          circuitDelay(selection);
          break;
        }
        if((currentMenu == 8) && (selection != 5)) //Selected to set purge time of a circuit
        {
          purgeConfig(selection);
          break;
        }
        if((currentMenu == 2) && (selection == 0)) //Selected to save a file
        {
          FileWriter();
          break;
        }
        if((currentMenu == 2) && (selection == 1)) //Selected to load a file
        {
          FileReader();
          break;
        }
        if((currentMenu == 2) && (selection == 2)) //Select to delete a file
        {
          FileRemover();
          break;
        }
        if((currentMenu == 3) && (selection != 8)) //Select to set pressure of a circuit or sensor
        {
          SetPressure(selection);
          break;
        }
        if((currentMenu == 7) && (selection != 3)) //Select to set PID variable of a circuit
        {
          setPID(circuitSelection, selection);
          break;
        }
        if((currentMenu == 0) && (selection == 0)) //Select automatic or manual mode
        {
          automaticMode = AutoManual();
          break;
        }
        if(menu[currentMenu][selection] == "EXIT")  //Select to exit menu
        {
          standbyMode = true;
          currentMenu = 0;
          lastSelection = 0;
          selection = 0;
          return;
        }

        //Using the selection, goes to the correct menu using the map, and sets the default option to the top
        currentMenu = menuMap[currentMenu][selection];
        lastSelection = selection;
        selection = 0; 
        break;
        
      case '2': //Menu Down
        selection++;
        if(selection >= limits[currentMenu])
        {
          selection = limits[currentMenu] - 1;
        }
        if(selection <= 0)
        {
          selection = 0;
        }
        if(currentMenu == 6)
        {
          circuitSelection = selection;
        }
        break;
        
      case '3': //Menu Up
        selection--;
        if(selection >= limits[currentMenu])
        {
          selection = limits[currentMenu] - 1;
        }
        if(selection <= 0)
        {
          selection = 0;
        }
        if(currentMenu == 6)
        {
          circuitSelection = selection;
        }
        break;
        
      case '4': //Return
        if(currentMenu >= 8)  //If we scroll to the last menu item, do not increment up
        {
          currentMenu = 8;
        }
        if(currentMenu < 0 || currentMenu == NULL)  //If we scroll to the first menu item, do not increment down
        {
          currentMenu = 0;
        }
        if(currentMenu != 0)  //Set the menu to the correct selection using the map
        {
          currentMenu = menuMap[currentMenu][limits[currentMenu]-1];
        }
        selection = lastSelection;    
        break;
        
      case '5': //Limited select
        if(currentMenu >= 7)  //If we scroll to the last menu item, do not increment up
        {
          currentMenu = 7;
        }
        if(currentMenu <= 0)  //If we scroll to the last menu item, do not increment down
        {
          currentMenu = 0;
        }
        if(menu[currentMenu][selection] == "EXIT")  //Select to exit menu
        {
          standbyMode = true;
          currentMenu = 0;
          lastSelection = 0;
          selection = 0;
          return;
        }
        //Using the selection, goes to the correct menu using the map, and sets the default option to the top
        if(menuMap[currentMenu][selection] != NULL)
        {
          currentMenu = menuMap[currentMenu][selection];
          lastSelection = selection;
          selection = 0; 
          break;
        }
    }
  }
  
  //Display the menu on the LCD and scroll two when the user reaches the top or bottom
  if(selection % 2 == 0)
  {
    lcd.setCursor(0,0);
    lcd.print(String(">" + menu[currentMenu][selection] + "                "));
    lcd.setCursor(0,1);
    lcd.print(String(" " + menu[currentMenu][selection+1] + "                "));
  }
  else
  {
    lcd.setCursor(0,0);
    lcd.print(String(" " + menu[currentMenu][selection-1] + "                "));
    lcd.setCursor(0,1);
    lcd.print(String(">" + menu[currentMenu][selection] + "                "));
  }

}

//-------------------------------------------------------------------------------------------------------------
//Auto/Manual selection. Boolean.
//-------------------------------------------------------------------------------------------------------------
bool AutoManual()  
{
  
  bool selecting = true;
  bool selection = automaticMode;

    ControlButtonStateManager();
  
  lcd.setCursor(0,0);
  lcd.print("SET CONTROL MODE");
  lcd.setCursor(0,1);
  if(selection)
  {
     lcd.print(">AUTO<   MANUAL "); 
  }
  else
  {
    lcd.print(" AUTO   >MANUAL<");
  }
  
  while(selecting)
  { 
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
      
      case '4': //Left
        lcd.setCursor(0,1);
        lcd.print(">AUTO<   MANUAL ");
        selection = true;
        break;
        
      case '5': //Right
        lcd.setCursor(0,1);
        lcd.print(" AUTO   >MANUAL<");
        selection = false;
        break;
      }
    }
  }

  //Display to user which mode is selected
  lcd.setCursor(0,0);
  if(selection)
  {
    lcd.print(" AUTOMATIC MODE ");
  }
  else
  {
    lcd.print("   MANUAL MODE  ");
  }
  lcd.setCursor(0,1);
  lcd.print("    ENABLED     ");
  delay(3000);
  
  return selection;
}

//-------------------------------------------------------------------------------------------------------------
//Yes or No selection. Boolean.
//-------------------------------------------------------------------------------------------------------------
bool YesOrNo()  
{
  
  bool selecting = true;
  bool selection = true;
      
  lcd.setCursor(0,1);
  lcd.print("  >YES<    NO   ");
  
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        return(selection);
        break;
      
      case '4': //Left
        lcd.setCursor(0,1);
        lcd.print("  >YES<    NO   ");
        selection = true;
        break;
        
      case '5': //Right
        lcd.setCursor(0,1);
        lcd.print("   YES    >NO<  ");
        selection = false;
        break;
      }
    }
  }
  return selection;
}

//-------------------------------------------------------------------------------------------------------------
//Updates the current settings to the file that is used when starting the program
//-------------------------------------------------------------------------------------------------------------
void SaveCurrentSettings()
{
  //Check is an SD card is present and active. If one is present, save the data. If not skip to return.
  if(sdCard && SD.begin())
  {
    SD.remove("Setting.txt");
    File lastPresetFile = SD.open("Setting.txt", FILE_WRITE);  //Save the setting
    lastPresetFile.println(alarmEnable);
    lastPresetFile.println(isCouple);
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
    lastPresetFile.close();
  }
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Save presets
//-------------------------------------------------------------------------------------------------------------
bool FileWriter()
{
  //Preset number variable. Defaults to one
  int presetNumber = 1;
  bool selecting = true;

    ControlButtonStateManager();
    
//menu select presets
  while(selecting)
  { 
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Menu Down
        presetNumber++;
        if(presetNumber > 10)
        {
          presetNumber = 1;
        }
        break;
        
      case '3': //Menu Up
        presetNumber--;
        if(presetNumber < 1)
        {
          presetNumber = 10;
        }
        break;
        
      case '4': //Return
        selecting = false;
        return false;
        break;
        
      }
    }
    lcd.setCursor(0,0);
    lcd.print("SELECT PRESET:  ");
    lcd.setCursor(0,1);
    lcd.print(String("Preset_" + String(presetNumber) + "                "));
  }
  
  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");

  //Save the selected preset
  SD.begin(csPin);
  if(SD.open(file, FILE_WRITE)) //File is found. Ask if overwrite.
  {
    lcd.setCursor(0,0);
    lcd.print(String("SAVE " + preset + "?                "));
    if(YesOrNo())
    {     
      SD.remove(file);
    
      File presetFile = SD.open(file, FILE_WRITE);  //Save the file

      lcd.setCursor(0,0);
      lcd.print((String("SAVING: " + preset + "                ")));
      lcd.setCursor(0,1);
      lcd.print("DO NOT POWER OFF");  //Corruption may occour is power is lost during save!

      presetFile.println(alarmEnable);
      presetFile.println(isCouple);
      presetFile.println(Marxsetpoint);
      presetFile.println(MTGsetpoint);
      presetFile.println(Switchsetpoint);
      presetFile.println(TG70Switchsetpoint);
      presetFile.println(TG70Marxsetpoint);
      presetFile.println(maxReclaimerPressure);
      presetFile.println(minReclaimerPressure);
      presetFile.println(marxenableState);
      presetFile.println(mtgenableState);
      presetFile.println(switchenableState);
      presetFile.println(tg70switchenableState);
      presetFile.println(tg70marxenableState);
      presetFile.println(marxmaxTime);
      presetFile.println(mtgmaxTime);
      presetFile.println(switchmaxTime);
      presetFile.println(tg70switchmaxTime);
      presetFile.println(tg70marxmaxTime);
      presetFile.println(marxDelay);
      presetFile.println(mtgDelay);
      presetFile.println(switchDelay);
      presetFile.println(tg70marxDelay);
      presetFile.println(tg70switchDelay);
      presetFile.println(marxPurgeTime);
      presetFile.println(mtgPurgeTime);
      presetFile.println(switchPurgeTime);
      presetFile.println(tg70switchPurgeTime);
      presetFile.println(tg70marxPurgeTime);
      presetFile.println(minBottlePressure);
      presetFile.println(kp_Marx);
      presetFile.println(ki_Marx);
      presetFile.println(kd_Marx);
      presetFile.println(kp_MTG);
      presetFile.println(ki_MTG);
      presetFile.println(kd_MTG);
      presetFile.println(kp_Switch);
      presetFile.println(ki_Switch);
      presetFile.println(kd_Switch);
      presetFile.println(kp_SwitchTG70);
      presetFile.println(ki_SwitchTG70);
      presetFile.println(kd_SwitchTG70);
      presetFile.println(kp_MarxTG70);
      presetFile.println(ki_MarxTG70);
      presetFile.println(kd_MarxTG70);
      presetFile.println(reclaimerSafetyTime);
      presetFile.close();

      delay(3000);
      SaveCurrentSettings();
      
      lcd.setCursor(0,0);
      lcd.print((String("SAVED: " + preset + "                ")));
      lcd.setCursor(0,1);
      lcd.print("SUCCESS         ");

      delay(3000);
      return true;
    }
    else
    {
      return false ;
    }
  }
  else //SD card is not found.
  {
    lcd.setCursor(0,0);
    lcd.print((String("ERROR: " + preset + "                ")));
    lcd.setCursor(0,1);
    lcd.print("SDCARD NOT FOUND");
    delay(3000);
    return true;
  }
  
  return true;
}

//-------------------------------------------------------------------------------------------------------------
//Load presets
//-------------------------------------------------------------------------------------------------------------
bool FileReader()
{
  //Preset number variable. Defaults to one
  int presetNumber = 1;
  bool selecting = true;

    ControlButtonStateManager();

//menu select presets
  while(selecting)
  {
    
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Menu Down
        presetNumber++;
        if(presetNumber > 10)
        {
          presetNumber = 1;
        }
        break;
        
      case '3': //Menu Up
        presetNumber--;
        if(presetNumber < 1)
        {
          presetNumber = 10;
        }
        break;
        
      case '4': //Return
        selecting = false;
        return false;
        break;
        
      }
    }
    lcd.setCursor(0,0);
    lcd.print("SELECT PRESET:  ");
    lcd.setCursor(0,1);
    lcd.print(String("Preset_" + String(presetNumber) + "                "));
  }

  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");
  
  SD.begin(csPin);
  if(SD.open(file, FILE_READ)) //File is found.
  {
    lcd.setCursor(0,0);
    lcd.print(String("LOAD " + preset + "?                "));
    if(YesOrNo())
    {     
      File presetFile = SD.open(file, FILE_READ);
        
      lcd.setCursor(0,0);
      lcd.print((String("LOAD: " + preset + "                ")));
      lcd.setCursor(0,1);
      lcd.print("DO NOT POWER OFF");  //Corruption may occour is power is lost during load!

      alarmEnable = presetFile.readStringUntil('\n').toInt();
      isCouple = presetFile.readStringUntil('\n').toInt();
      Marxsetpoint = presetFile.readStringUntil('\n').toDouble();
      MTGsetpoint = presetFile.readStringUntil('\n').toDouble();
      Switchsetpoint = presetFile.readStringUntil('\n').toDouble();
      TG70Switchsetpoint = presetFile.readStringUntil('\n').toDouble();
      TG70Marxsetpoint = presetFile.readStringUntil('\n').toDouble();
      maxReclaimerPressure = presetFile.readStringUntil('\n').toDouble();
      minReclaimerPressure = presetFile.readStringUntil('\n').toDouble();
      marxenableState = presetFile.readStringUntil('\n').toInt();
      mtgenableState = presetFile.readStringUntil('\n').toInt();
      switchenableState = presetFile.readStringUntil('\n').toInt();
      tg70switchenableState = presetFile.readStringUntil('\n').toInt();
      tg70marxenableState = presetFile.readStringUntil('\n').toInt();
      marxmaxTime = presetFile.readStringUntil('\n').toInt();
      mtgmaxTime = presetFile.readStringUntil('\n').toInt();
      switchmaxTime = presetFile.readStringUntil('\n').toInt();
      tg70switchmaxTime = presetFile.readStringUntil('\n').toInt();
      tg70marxmaxTime = presetFile.readStringUntil('\n').toInt();
      marxDelay = presetFile.readStringUntil('\n').toInt();
      mtgDelay = presetFile.readStringUntil('\n').toInt();
      switchDelay = presetFile.readStringUntil('\n').toInt();
      tg70marxDelay = presetFile.readStringUntil('\n').toInt();
      tg70switchDelay = presetFile.readStringUntil('\n').toInt();
      marxPurgeTime = presetFile.readStringUntil('\n').toInt();
      mtgPurgeTime = presetFile.readStringUntil('\n').toInt();
      switchPurgeTime = presetFile.readStringUntil('\n').toInt();
      tg70switchPurgeTime = presetFile.readStringUntil('\n').toInt();
      tg70marxPurgeTime = presetFile.readStringUntil('\n').toInt();
      minBottlePressure = presetFile.readStringUntil('\n').toDouble();
      kp_Marx = presetFile.readStringUntil('\n').toDouble();
      ki_Marx = presetFile.readStringUntil('\n').toDouble();
      kd_Marx = presetFile.readStringUntil('\n').toDouble();
      kp_MTG = presetFile.readStringUntil('\n').toDouble();
      ki_MTG = presetFile.readStringUntil('\n').toDouble();
      kd_MTG = presetFile.readStringUntil('\n').toDouble();
      kp_Switch = presetFile.readStringUntil('\n').toDouble();
      ki_Switch = presetFile.readStringUntil('\n').toDouble();
      kd_Switch = presetFile.readStringUntil('\n').toDouble();
      kp_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      ki_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      kd_SwitchTG70 = presetFile.readStringUntil('\n').toDouble();
      kp_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      ki_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      kd_MarxTG70 = presetFile.readStringUntil('\n').toDouble();
      //reclaimerSafetyTime = presetFile.readStringUntil('\n').toInt();

      lastmarxenableState = !marxenableState;
      lastmtgenableState = !mtgenableState;
      lastswitchenableState = !switchenableState;
      lasttg70switchenableState = !tg70switchenableState;
      lasttg70marxenableState = !tg70marxenableState;

      presetFile.close();

      delay(3000);
      SaveCurrentSettings();

      lcd.setCursor(0,0);
      lcd.print((String("LOAD: " + preset + "                ")));
      lcd.setCursor(0,1);
      lcd.print("SUCCESS         ");

      delay(3000);
      return true;
    }
    else
    {
      return false;
    }
  }
  else //No file is found, display error and return to menu
  {
    lcd.setCursor(0,0);
    lcd.print((String("ERROR: " + preset + "                ")));
    lcd.setCursor(0,1);
    lcd.print("FILE NOT FOUND  ");

    delay(3000);
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------------------
//Delete presets.
//-------------------------------------------------------------------------------------------------------------
bool FileRemover()
{
  int presetNumber = 1;
  bool selecting = true;

    ControlButtonStateManager();

//menu select presets
  while(selecting)
  { 
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Menu Down
        presetNumber++;
        if(presetNumber > 10)
        {
          presetNumber = 1;
        }
        break;
        
      case '3': //Menu Up
        presetNumber--;
        if(presetNumber < 1)
        {
          presetNumber = 10;
        }
        break;
        
      case '4': //Return
        selecting = false;
        return false;
        break;
        
      }
    }
    lcd.setCursor(0,0);
    lcd.print("SELECT PRESET:  ");
    lcd.setCursor(0,1);
    lcd.print(String("Preset_" + String(presetNumber) + "                "));
  }

  String preset = String("Preset_" + String(presetNumber));
  String file = String(preset + ".txt");
  
  SD.begin(csPin);
  if(SD.exists(file)) //File is found.
  {
    lcd.setCursor(0,0);
    lcd.print(String("DEL " + preset + "?                "));
    if(YesOrNo())
    {     
      SD.remove(file);

      lcd.setCursor(0,0);
      lcd.print((String("DEL: " + preset + "                ")));
      lcd.setCursor(0,1);
      lcd.print("SUCCESS         ");

      delay(3000);
      return true;
    }
    else
    {
      return false;
    }
  }
  else //No file is found, display error and return to menu
  {
    lcd.setCursor(0,0);
    lcd.print((String("ERROR: " + preset + "                ")));
    lcd.setCursor(0,1);
    lcd.print("FILE NOT FOUND  ");

    delay(3000);
    return false;
  }
  
  return true;
}

//-------------------------------------------------------------------------------------------------------------
//Set the pressure of a given circuit to the current reading.
//-------------------------------------------------------------------------------------------------------------
void SetPressure(int selection)  
{
  bool selecting = true;
  bool set = true;
  int oldValue = 0;
  int newValuePin = 0;
  String selString = " >SET<  CANCEL  ";

      ControlButtonStateManager();

  lcd.setCursor(0,0);
  switch (selection)
  {
    //Marx
    case 0:
      newValuePin = marxanaloginPin;
      oldValue = int(Marxsetpoint);
      break;

    //Marx TG70
    case 1:
      newValuePin = tg70marxanaloginPin;
      oldValue = int(TG70Marxsetpoint);
      break;

    //MTG
    case 2:
      newValuePin = mtganaloginPin;
      oldValue = int(MTGsetpoint);
      break;

    //Switch
    case 3:
      newValuePin = switchanaloginPin;
      oldValue = int(Switchsetpoint);
      break;

    //Switch TG70
    case 4:
      newValuePin = tg70switchanaloginPin;
      oldValue = int(TG70Switchsetpoint);
      break;

    //Recaimer On
    case 5:
      newValuePin = reclaimeranaloginPin;
      oldValue = int(maxReclaimerPressure);
      break;

    //Reclaimer Off
    case 6:
      newValuePin = reclaimeranaloginPin;
      oldValue = int(minReclaimerPressure);
      break;

    //Minimum Supply
    case 7:
      newValuePin = bottleanaloginPin;
      oldValue = int(minBottlePressure);
      break;
  }
  
  while(selecting)
  {         
    lcd.print(String("NEW:" + String(int(analogRead(newValuePin))) + "                 "));
    lcd.setCursor(8,0);
    lcd.print(String(" OLD:" + String(oldValue) + "                 "));
    lcd.setCursor(0,1);
    lcd.print(selString);
    
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
      
      case '4': //Left
        selString = " >SET<  CANCEL  ";
        set = true;
        break;
        
      case '5': //Right
        selString = "  SET  >CANCEL< ";
        set = false;
        break;
      }
    }
  }
  
  if(!set)
  {
    return;
  }
  
  lcd.setCursor(0,0);
  lcd.print("SETTING:        ");
  lcd.setCursor(0,1);
  
  //Selection of which circuit the user would like to save
  switch (selection)
  {
    case 0:
      Marxsetpoint = analogRead(marxanaloginPin);
      lcd.print(String("MARX: " + String(Marxsetpoint) + "                "));
      break;
    
    case 1:
      TG70Marxsetpoint = analogRead(tg70marxanaloginPin);
      lcd.print(String("MARX TG70: " + String(TG70Marxsetpoint) + "                "));
      break;
      
    case 2:
      MTGsetpoint = analogRead(mtganaloginPin);
      lcd.print(String("MTG: " + String(MTGsetpoint) + "                "));
      break;
      
    case 3:
      Switchsetpoint = analogRead(switchanaloginPin);
      lcd.print(String("SWITCH: " + String(Switchsetpoint) + "                "));
      break;
            
    case 4:
      TG70Switchsetpoint = analogRead(tg70switchanaloginPin);
      lcd.print(String("SWITCH TG70: " + String(TG70Switchsetpoint) + "                "));
      break;
      
    case 5:
      if((analogRead(reclaimeranaloginPin) - minReclaimerPressure <= 100) && (analogRead(reclaimeranaloginPin) - minReclaimerPressure >= 0))
      {
        lcd.setCursor(0,0);
        lcd.print("ERROR: RECLAIMER");
        lcd.setCursor(0,1);
        lcd.print("WINDOW TOO SMALL");     
      }
      if(analogRead(reclaimeranaloginPin) - minReclaimerPressure < 0)
      {
        lcd.setCursor(0,0);
        lcd.print("ERROR: RECLAIMER");
        lcd.setCursor(0,1);
        lcd.print("WINDOW NEGATIVE ");     
      }
      else
      {
        maxReclaimerPressure = analogRead(reclaimeranaloginPin);
        lcd.print(String("RECL. ON: " + String(maxReclaimerPressure) + "                "));
      }
      break;
                  
    case 6:
      if((maxReclaimerPressure - analogRead(reclaimeranaloginPin) <= 100) && (analogRead(reclaimeranaloginPin) - minReclaimerPressure >= 0))
      {
        lcd.setCursor(0,0);
        lcd.print("ERROR: RECLAIMER");
        lcd.setCursor(0,1);
        lcd.print("WINDOW TOO SMALL");     
      }
      if(maxReclaimerPressure - analogRead(reclaimeranaloginPin)< 0)
      {
        lcd.setCursor(0,0);
        lcd.print("ERROR: RECLAIMER");
        lcd.setCursor(0,1);
        lcd.print("WINDOW NEGATIVE ");     
      }
      else
      {
        minReclaimerPressure = analogRead(reclaimeranaloginPin);
        lcd.print(String("RECL. OFF: " + String(minReclaimerPressure) + "                "));
      }
      break;

    case 7:
      minBottlePressure = analogRead(bottleanaloginPin);
      lcd.print(String("MIN SUPPLY: " + String(minBottlePressure) + "                "));
      break;
  }
  delay(3000);
  SaveCurrentSettings();
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
    alarmController(String("LOW SUPPLY      "));
  }
}

//-------------------------------------------------------------------------------------------------------------
//Purge time setting.
//-------------------------------------------------------------------------------------------------------------
void purgeConfig(int selection)
{

  long int setTime = 0;
  int digit = 0;
  int decimal = 5;
  long int number[3] = {0,0,0};
  bool selecting = true;
  bool displayDigit = true;

    ControlButtonStateManager();
    
  //Check which circuit we are setting the alarm time for, and retrieve the corresponidng value
  switch (selection)
  {
    case 0:
      setTime = marxPurgeTime;
      break;
    
    case 1:
      setTime = tg70marxPurgeTime;
      break;
      
    case 2:
      setTime = mtgPurgeTime;
      break;
      
    case 3:
      setTime = switchPurgeTime;
      break;
            
    case 4:
      setTime = tg70switchPurgeTime;
      break;
  }

  lcd.setCursor(0,0);
  lcd.print("SET PURGE TIME: ");
  lcd.setCursor(0,1);

  //Break the given time into individual digits
  number[0] = setTime / 100000 % 10;
  number[1] = setTime / 10000 % 10;
  number[2] = setTime / 1000 % 10;

  //Put the individual digitso nto the LCD
  lcd.print("   ");
  lcd.setCursor(3,1);
  lcd.print(String(number[0]));
  lcd.setCursor(4,1);
  lcd.print(String(number[1]));
  lcd.setCursor(5,1);
  lcd.print(String(number[2]));  
  lcd.setCursor(6,1);
  lcd.print(" SECONDS  ");  
  lcd.setCursor(decimal, 1);

  digit = number[2];

  //Set the time based n user input
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Down
        digit--;
        if(digit < 0)
        {
          digit = 9;
        }
        number[decimal - 3] = digit;
        lcd.print(digit);
        break;
        
      case '3': //Up
        digit++;
        if(digit > 9)
        {
          digit = 0;
        }
        number[decimal - 3] = digit;
        lcd.print(digit);
        break;
        
      case '4': //Left
        lcd.print(digit);
        decimal--;
        if(decimal < 3)
        {
          decimal = 3;
        }
        digit = number[decimal - 3];
        break;
        
      case '5': //Right
        lcd.print(digit);
        decimal++;
        if(decimal > 5)
        {
          decimal = 5;
        }
        digit = number[decimal - 3];
        break;
      }
    }

    lcd.setCursor(decimal, 1);

    //Blink the character space to let the user know what digit they are setting
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(displayDigit)
      {
        displayDigit = false;
        lcd.print("_");
      }
      else
      {
        displayDigit = true;
        lcd.print(digit);
      }
    }
  }

  //Calculate the new purge time from the selection
  setTime = (number[0]*100 + number[1]*10 + number[2]) * 1000;
  SaveCurrentSettings();

  //Set the new purge time to the correct circuit
  lcd.setCursor(0,0);
  switch (selection)
  {
    case 0:
      marxPurgeTime = setTime;
      lcd.print("MARX PURGE:     ");
      break;
          
    case 1:
      tg70marxPurgeTime = setTime;
      lcd.print("MARX TG70 PURGE: ");
      break;
  
    case 2:
      mtgPurgeTime = setTime;
      lcd.print("MTG PURGE:      ");
      break;
      
    case 3:
      switchPurgeTime = setTime;
      lcd.print("SWITCH PURGE:   ");
      break;
            
    case 4:
      tg70switchPurgeTime = setTime;
      lcd.print("SWITCH TG70 PURGE");
      break;
  }
  
  lcd.setCursor(0,1);
  lcd.print(String(String(setTime / 1000) + " SECONDS        "));
  delay(3000);
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Circuit delay configuration function
//-------------------------------------------------------------------------------------------------------------
void circuitDelay(int selection)
{
  
  long int setTime = 0;
  int digit = 0;
  int decimal = 3;
  long int number[4] = {0,0,0,0};
  bool selecting = true;
  bool displayDigit = true;

    ControlButtonStateManager();

  //Check which circuit we are setting the alarm time for, and retrieve the corresponidng value
  switch (selection)
  {
    case 0:
      setTime = marxDelay;
      break;
    
    case 1:
      setTime = tg70marxDelay;
      break;
      
    case 2:
      setTime = mtgDelay;
      break;
      
    case 3:
      setTime = switchDelay;
      break;
            
    case 4:
      setTime = tg70switchDelay;
      break;
  }

  
  lcd.setCursor(0,0);
  lcd.print("SET DELAY TIME: ");
  lcd.setCursor(0,1);

  //Break the given time into individual digits
  number[0] = setTime / 1000 % 10;
  number[1] = setTime / 100 % 10;
  number[2] = setTime / 10 % 10;
  number[3] = setTime / 1 % 10;

  //Put the individual digitso nto the LCD
  lcd.setCursor(0,1);
  lcd.print(String(number[0]));
  lcd.setCursor(1,1);
  lcd.print(String(number[1]));
  lcd.setCursor(2,1);
  lcd.print(String(number[2]));  
  lcd.setCursor(3,1);
  lcd.print(String(number[3]));  
  lcd.setCursor(4,1);
  lcd.print(" MILLISECONDS  ");  
  lcd.setCursor(decimal, 1);

  digit = number[3];

  //Set the time based on user input
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Down
        digit--;
        if(digit < 0)
        {
          digit = 9;
        }
        number[decimal] = digit;
        lcd.print(digit);
        break;
        
      case '3': //Up
        digit++;
        if(digit > 9)
        {
          digit = 0;
        }
        number[decimal] = digit;
        lcd.print(digit);
        break;
        
      case '4': //Left
        lcd.print(digit);
        decimal--;
        if(decimal < 0)
        {
          decimal = 0;
        }
        digit = number[decimal];
        break;
        
      case '5': //Right
        lcd.print(digit);
        decimal++;
        if(decimal > 3)
        {
          decimal = 3;
        }
        digit = number[decimal];
        break;
      }
    }

    lcd.setCursor(decimal, 1);

    //Blink the character space to let the user know what digit they are setting
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(displayDigit)
      {
        displayDigit = false;
        lcd.print("_");
      }
      else
      {
        displayDigit = true;
        lcd.print(digit);
      }
    }
  }

  //Calculate the new time from the selection
  setTime = (number[0]*1000 + number[1]*100 + number[2]*10 + number[3]);
  SaveCurrentSettings();
  
  //Set the new alarm time to the correct circuit
  lcd.setCursor(0,0);
  switch (selection)
  {
    case 0:
      marxDelay = setTime;
      lcd.print("MARX DELAY:     ");
      break;
          
    case 1:
      tg70marxDelay = setTime;
      lcd.print("MARX TG70 DELAY: ");
      break;
  
    case 2:
      mtgDelay = setTime;
      lcd.print("MTG DELAY:      ");
      break;
      
    case 3:
      switchDelay = setTime;
      lcd.print("SWITCH DELAY:   ");
      break;
            
    case 4:
      tg70switchDelay = setTime;
      lcd.print("SWITCH TG70 DELAY");
      break;
  }

  lcd.setCursor(0,1);
  lcd.print(String(String(setTime) + " MILLISECONDS    "));
  delay(3000);
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Reclaimer safety delay setting function
//-------------------------------------------------------------------------------------------------------------
void setReclaimerSafetyDelay()
{
  
  long int setTime = reclaimerSafetyTime / 60000;
  bool selecting = true;
  bool displayDigit = true;

    ControlButtonStateManager();
  lcd.setCursor(0,0);
  lcd.print("SET DELAY TIME: ");
  lcd.setCursor(0,1);
  lcd.print(String(setTime) + " MINUTES        "); //Display time in minutes

  //Set the time based on user input
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Down
        setTime--;
        if(setTime < 1)
        {
          setTime = 5;
        }
        break;
        
      case '3': //Up
        setTime++;
        if(setTime > 5)
        {
          setTime = 1;
        }
        break;
      }  
      lcd.setCursor(0,1);
      lcd.print(String(setTime) + " MINUTES       "); //Display time in minutes
    }

    //Blink the character space to let the user know what digit they are setting
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(displayDigit)
      {
        displayDigit = false;
        lcd.setCursor(0,1);
        lcd.print("_ MINUTES       "); //Display time in minutes
      }
      else
      {
        displayDigit = true;
        lcd.setCursor(0,1);
        lcd.print(String(setTime) + " MINUTES       "); //Display time in minutes
      }
    }
  }

  //Set and save the new delay time
  reclaimerSafetyTime = setTime * 60000;
  SaveCurrentSettings();
  lcd.setCursor(0,0);
  lcd.print("REC SAFETY TIME:");
  lcd.setCursor(0,1);
  lcd.print(String(setTime) + " MINUTES        ");
  delay(3000);
  return;
}


//-------------------------------------------------------------------------------------------------------------
//Set the selected PID variables for a given circuit
//-------------------------------------------------------------------------------------------------------------
void setPID(int selection, int tuneVariable)
{

  String circuit;
  String PIDvariableName;
  double kp = 0;
  double ki = 0;
  double kd = 0;
  int setPID = 0;
  int digit = 0;
  int decimal = 9;
  long int number[4] = {0,0,0,0};
  bool selecting = true;
  bool displayDigit = true;

    ControlButtonStateManager();
    
  //Check which circuit we are setting the PID for, and retrieve the corresponidng value
  switch (selection)
  {
    case 0:
      kp = kp_Marx;
      ki = ki_Marx;
      kd = kd_Marx;
      circuit = "MARX";
      break;
    
    case 1:
      kp = kp_MarxTG70;
      ki = ki_MarxTG70;
      kd = kd_MarxTG70;
      circuit = "MARX TG70";
      break;
      
    case 2:
      kp = kp_MTG;
      ki = ki_MTG;
      kd = kd_MTG;
      circuit = "MTG";
      break;
      
    case 3:
      kp = kp_Switch;
      ki = ki_Switch;
      kd = kd_Switch;
      circuit = "SWITCH";
      break;
            
    case 4:
      kp = kp_SwitchTG70;
      ki = ki_SwitchTG70;
      kd = kd_SwitchTG70;
      circuit = "SWITCH TG70";
      break;
  }

  //Set the correct PID variable to change
  switch (tuneVariable)
  {
    case 0:
      setPID = kp * 100;
      PIDvariableName = " KP: ";
      break;
      
    case 1:
      setPID = ki * 100;
      PIDvariableName = " KI: ";
      break;

    case 2:
      setPID = kd * 100;
      PIDvariableName = " KD: ";
      break;
  }

  
  lcd.setCursor(0,0);
  lcd.print(String(String("SET " + circuit) + PIDvariableName));

  //Break the given time into individual digits
  number[0] = (setPID / 1000) % 10;
  number[1] = (setPID / 100) % 10;
  number[2] = (setPID / 10) % 10;
  number[3] = (setPID / 1) % 10;

  //Put the individual digitso nto the LCD
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.print(PIDvariableName);
  lcd.setCursor(5,1);
  lcd.print(String(number[0]));
  lcd.setCursor(6,1);
  lcd.print(String(number[1]));
  lcd.setCursor(7,1);
  lcd.print(".");
  lcd.setCursor(8,1);
  lcd.print(String(number[2]));  
  lcd.setCursor(9,1);
  lcd.print(String(number[3]));   
  lcd.setCursor(decimal, 1); 

  digit = number[3];

  //Set the time based on user input
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Down
        digit--;
        if(digit < 0)
        {
          digit = 9;
        }
        if(digit > 9)
        {
          digit = 0;
        }
        if(decimal <= 6)
        {
          number[decimal - 5] = digit;
        }
        else
        {
          number[decimal - 6] = digit;
        }
        lcd.print(digit);
        break;
        
      case '3': //Up
        digit++;
        if(digit > 9)
        {
          digit = 0;
        }
        if(decimal <= 6)
        {
          number[decimal - 5] = digit;
        }
        else
        {
          number[decimal - 6] = digit;
        }
        lcd.print(digit);
        break;
        
      case '4': //Left
        lcd.print(digit);
        decimal--;
        if(decimal < 5)
        {
          decimal = 5;
        }
        if(decimal == 7)
        {
          decimal = 6;
        }
        if(decimal <= 6)
        {
          digit = number[decimal - 5];
        }
        else
        {
          digit = number[decimal - 6];
        }
        break;
        
      case '5': //Right
        lcd.print(digit);
        decimal++;
        if(decimal > 9)
        {
          decimal = 9;
        }
        if(decimal == 7)
        {
          decimal = 8;
        }
        if(decimal <= 6)
        {
          digit = number[decimal - 5];
        }
        else
        {
          digit = number[decimal - 6];
        }
        break;
      }
    }
    
    lcd.setCursor(7,1);
    lcd.print(".");
    lcd.setCursor(decimal, 1);

    //Blink the character space to let the user know what digit they are setting
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(displayDigit)
      {
        displayDigit = false;
        lcd.print("_");
      }
      else
      {
        displayDigit = true;
        lcd.print(digit);
      }
    }
  }

  //Calculate the new time from the selection
  double newPID = (number[0]*10 + number[1] + number[2]/10 + number[3]/100);
  
  //Set the correct PID variable
  switch (tuneVariable)
  {
    case 0:
      kp = newPID;
      break;
      
    case 1:
      ki = newPID;
      break;

    case 2:
      kd = newPID;
      break;
  }
    switch (selection)
  {
    case 0:
      kp_Marx = kp;
      ki_Marx = ki;
      kd_Marx = kd;
      break;
    
    case 1:
      kp_MarxTG70 = kp;
      ki_MarxTG70 = ki;
      kd_MarxTG70 = kd;
      break;
      
    case 2:
      kp_MTG = kp;
      ki_MTG = ki;
      kd_MTG = kd;
      break;
      
    case 3:
      kp_Switch = kp;
      ki_Switch = ki;
      kd_Switch = kd;
      break;
            
    case 4:
      kp_SwitchTG70 = kp;
      ki_SwitchTG70 = ki;
      kd_SwitchTG70 = kd;
      break;
  }
  SaveCurrentSettings();
  
  //Set the new PID parameter to the correct circuit PID
  lcd.setCursor(0,0);
  lcd.print(String(String(circuit + PIDvariableName) + "                "));
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(6,1);
  lcd.print(newPID);
  
  delay(3000);
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Alarm on/off
//-------------------------------------------------------------------------------------------------------------
void setAlarmOnOff()  
{
  
  bool selecting = true;
      
  ControlButtonStateManager();
      
  //menu select presets
  lcd.setCursor(0,0);
  lcd.print("SET ALARM SOUND:");
  lcd.setCursor(0,1);
  if(alarmEnable)
  {
    lcd.print("  >ON<    OFF   ");
  }
  else
  {
    lcd.print("   ON    >OFF<  ");
  }
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
      
      case '4': //Left
        lcd.setCursor(0,1);
        lcd.print("  >ON<    OFF   ");
        alarmEnable = true;
        break;
        
      case '5': //Right
        lcd.setCursor(0,1);
        lcd.print("   ON    >OFF<  ");
        alarmEnable = false;
        break;
      }
    }
  }
  
  SaveCurrentSettings();
  lcd.setCursor(0,0);
  lcd.print("  ALARM SOUND:  ");
  lcd.setCursor(0,1);
  if(alarmEnable)
  {
    lcd.print("       ON       ");
  }
  else
  {
    lcd.print("      OFF       ");
  }
  delay(3000);
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Alarm configuration function for circuit timeout
//-------------------------------------------------------------------------------------------------------------
void alarmConfig(int selection)
{
  
  long int setTime = 0;
  int digit = 0;
  int decimal = 5;
  long int number[3] = {0,0,0};
  bool selecting = true;
  bool displayDigit = true;

    ControlButtonStateManager();
    
  //Check which circuit we are setting the alarm time for, and retrieve the corresponidng value
  switch (selection)
  {
    case 1:
      setTime = marxmaxTime;
      break;
    
    case 2:
      setTime = tg70marxmaxTime;
      break;
      
    case 3:
      setTime = mtgmaxTime;
      break;
      
    case 4:
      setTime = switchmaxTime;
      break;
            
    case 5:
      setTime = tg70switchmaxTime;
      break;
  }

  
  lcd.setCursor(0,0);
  lcd.print("SET ALARM TIME: ");
  lcd.setCursor(0,1);

  //Break the given time into individual digits
  number[0] = setTime / 100000 % 10;
  number[1] = setTime / 10000 % 10;
  number[2] = setTime / 1000 % 10;

  //Put the individual digitso nto the LCD
  lcd.print("   ");
  lcd.setCursor(3,1);
  lcd.print(String(number[0]));
  lcd.setCursor(4,1);
  lcd.print(String(number[1]));
  lcd.setCursor(5,1);
  lcd.print(String(number[2]));  
  lcd.setCursor(6,1);
  lcd.print(" SECONDS  ");  
  lcd.setCursor(decimal, 1);

  digit = number[2];

  //Set the time based n user input
  while(selecting)
  {
    char key = keypad.getKey();
    if(key)
    {
      switch (key)
      {
      case '1': //Select
        selecting = false;
        break;
        
      case '2': //Down
        digit--;
        if(digit < 0)
        {
          digit = 9;
        }
        number[decimal - 3] = digit;
        lcd.print(digit);
        break;
        
      case '3': //Up
        digit++;
        if(digit > 9)
        {
          digit = 0;
        }
        number[decimal - 3] = digit;
        lcd.print(digit);
        break;
        
      case '4': //Left
        lcd.print(digit);
        decimal--;
        if(decimal < 3)
        {
          decimal = 3;
        }
        digit = number[decimal - 3];
        break;
        
      case '5': //Right
        lcd.print(digit);
        decimal++;
        if(decimal > 5)
        {
          decimal = 5;
        }
        digit = number[decimal - 3];
        break;
      }
    }

    lcd.setCursor(decimal, 1);

    //Blink the character space to let the user know what digit they are setting
    unsigned long currentTime = millis();
    if(currentTime - previousTime >= 500)
    {
      previousTime = currentTime;
      if(displayDigit)
      {
        displayDigit = false;
        lcd.print("_");
      }
      else
      {
        displayDigit = true;
        lcd.print(digit);
      }
    }
  }

  //Calculate the new time from the selection
  setTime = (number[0]*100 + number[1]*10 + number[2]) * 1000;
  SaveCurrentSettings();
  
  //Set the new alarm time to the correct circuit
  lcd.setCursor(0,0);
  switch (selection)
  {
    case 1:
      marxmaxTime = setTime;
      lcd.print("MARX ALARM:     ");
      break;
          
    case 2:
      tg70marxmaxTime = setTime;
      lcd.print("MARX TG70 ALARM: ");
      break;
  
    case 3:
      mtgmaxTime = setTime;
      lcd.print("MTG ALARM:      ");
      break;
      
    case 4:
      switchmaxTime = setTime;
      lcd.print("SWITCH ALARM:   ");
      break;
            
    case 5:
      tg70switchmaxTime = setTime;
      lcd.print("SWITCH TG70 ALARM");
      break;
  }

  lcd.setCursor(0,1);
  lcd.print(String(String(setTime / 1000) + " SECONDS        "));
  delay(3000);
  return;
}

//-------------------------------------------------------------------------------------------------------------
//Alarm controller checks if condition for alarm is met
//-------------------------------------------------------------------------------------------------------------
void alarmController(String errorString)
{
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
    lcd.setCursor(0,0);
    lcd.print("ALARM:          ");
    lcd.setCursor(0,1);
    lcd.print(errorString);
  }
  lcd.begin(16, 2);
  
  //User has pressed the alarm button. Turn off the alarm, display error to user, LED blinks
  while(errorState && alarmState)   
  {
    standbyMode = false;
      ControlButtonStateManager();
    digitalWrite(alarmsoundPin,LOW);  
    lcd.setCursor(0,0);
    lcd.print("CHECK ALARM:    ");
    lcd.setCursor(0,1);
    lcd.print(errorString);

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
    lcd.setCursor(0,0);
    lcd.print("ALARM CLEARED   ");
    lcd.setCursor(0,1);
    lcd.print(errorString);
    delay(3000);
  }
}

//-------------------------------------------------------------------------------------------------------------
//Reset LCD after 1 second intervals
//-------------------------------------------------------------------------------------------------------------
void resetLCD()
{
  unsigned long currentTime = millis();
  if(currentTime - lcdResetTime >= 1000)
  {
    lcdResetTime = currentTime;
    lcd.begin(16, 2);
   }
}
