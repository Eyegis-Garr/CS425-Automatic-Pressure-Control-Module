#include <Arduino.h>
#include <EasyNextionLibrary.h>
#include "simulator.h"

//EasyNex d(Serial3);

char sys_bytes[32];
char print_buf[64];

char cmap[C_NUM_CIRCUITS][16] = {
  "MARX",
  "MTG70",
  "MTG",
  "SWITCH",
  "SWTG70"
};

char pmap[C_NUM_PARAM][32] = {
  "PRESSURE",
  "SET POINT",
  "MAX. TIME",
  "CHECK TIME",
  "PURGE TIME",
  "DELAY TIME",
  "PID KP",
  "PID KI",
  "PID KD",
  "ALARM"
};

void setup() {
  // put your setup code here, to run once:
  sim_setup();

//  d.begin(9600);

  Serial.begin(9600);

  pinMode(16, OUTPUT);

  digitalWrite(16, LOW);

  sys.ui.nex->writeStr("page Global");  
}

void loop() {
//  sim_tick();

  digitalWrite(16, LOW);

//  d.NextionListen();
  sys.ui.nex->NextionListen();

  if (sys.ui.cmask && sys.ui.pmask) {
    for (int i = 0; i < C_NUM_CIRCUITS; i += 1) {
      if (isbset(sys.ui.cmask, i)) {
        sys.ui.cmask = 0;
        sprintf(print_buf, "-=== Circuit %s selected ===-", cmap[i]);
        Serial.println(print_buf);
        for (int k = 0; k < C_NUM_PARAM; k += 1) {
          if (isbset(sys.ui.pmask, k)) {
            sprintf(print_buf, "\tPARAMETER -> %s ", pmap[k]);
            Serial.print(print_buf); Serial.println(sys.circuits[i].params[k]);
            sys.ui.pmask = 0;
          }
        }
      }
    }
  }
}
