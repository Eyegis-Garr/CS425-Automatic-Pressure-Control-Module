#include <Arduino.h>
#include <inttypes.h>

#include "simulator.h"

void setup() {
  sim_setup();
  
  Serial.begin(9600);
}

void loop() {
  sim_tick();
}
