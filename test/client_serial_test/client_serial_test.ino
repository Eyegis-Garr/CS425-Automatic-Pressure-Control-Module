#include <AltSoftSerial.h>

AltSoftSerial xbee;

void setup() {
  xbee.begin(9600);
  Serial.begin(9600);
}

void loop() {
  if (xbee.available()) {
    Serial.println((uint8_t)xbee.read());
  }
}
