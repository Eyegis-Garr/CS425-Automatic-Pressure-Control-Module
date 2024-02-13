#include <Arduino.h>
#include <inttypes.h>

#include "simulator.h"

uint8_t pbuf[128];
packet_t p_rx;

void setup() {
  sim_setup();
  Serial1.begin(9600);
  Serial.begin(9600);

  p_rx.bytes = pbuf;
  // set updates to send (want to be able to set this via client commands)
  sys.up_types = UP_SYSTEM | UP_CIRCUITS;
  // construct and transmit updates
  int b = issue_updates(&Serial1);
  Serial.print("TX BYTES: "); Serial.println(b);
}

void loop() {
  // sim_tick();
  if (Serial1.available()) {
    if (Serial1.read() == SOTX) {
      int r = rx_packet(&Serial1, &p_rx);
      if (r) {
        Serial.print("RX BYTES: "); Serial.println(r);
        Serial.print("PK TYPE: "); Serial.println(p_rx.type);
        Serial.print("PK FLAGS: "); Serial.println(p_rx.flags);
        Serial.print("PK SIZE: "); Serial.println(p_rx.size);
        for (int i = 0; i < p_rx.size; i += 1) {
          Serial.println(p_rx.bytes[i]);
        }
      }
    }
  }
}
