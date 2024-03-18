#include <Arduino.h>
#include <inttypes.h>

#include "simulator.h"

void setup() {
  sim_setup();

  delay(100);
}

void loop() {
  sim_tick();
}
