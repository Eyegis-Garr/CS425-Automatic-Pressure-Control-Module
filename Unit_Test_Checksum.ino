//Sean Rolandelli

#include "CRC8.h"
#include "CRC.h"
#include "AUnit.h"

CRC8 crc;

test() {
  char *oldSettings[2] = {"hi", "hello"};
  char *currentSettings[2] = {"hi", "hello"};

  crc.reset();

  int old = 0, current = 0, i = 0;

  while(i < sizeof(oldSettings)/sizeof(int)) {
    old = calcCRC8((uint8_t *)oldSettings[i], 9);
    current = calcCRC8((uint8_t *)currentSettings[i], 9);
    i++;

    if(old != current) {
      failTestNow();
    }
  }
}

void setup() {
  Serial.begin(9600);
}

void loop() {
    aunit::TestRunner::run();
    return 0;
}
