#include "EasyNextionLibrary.h"

EasyNex myNex(Serial1);

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  
  myNex.begin(9600);

  delay(3000);
  
  Serial1.print("page 16");
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);

  delay(3000);

  Serial1.print("page 1");
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
}

void loop() {
  myNex.NextionListen();
}

void trigger0() {
  Serial.println("hi");
}
