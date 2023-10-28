#include "Arduino.h"


class SetPressure
{
  int relayPin, sensorPin;
  double currentPressure, threshold;
  int window,calculatedwindow;
  bool highorlow;

  public:
  //Constructor
SetPressure(int relayPin, int sensorPin, double currentPressure, double threshold, int window, int calculated window, bool highorlow)
{
  relayPin = relayPin;
  sensorPin = sensorPin;
  currentPressure = currentPressure;
  
  
    
}
  
