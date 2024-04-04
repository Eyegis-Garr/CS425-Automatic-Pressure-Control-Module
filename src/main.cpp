#include <Arduino.h>
#include <inttypes.h>

#include "simulator.h"

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  
  sim_setup();

  delay(100);
}

void loop() {
  sim_tick();
}
