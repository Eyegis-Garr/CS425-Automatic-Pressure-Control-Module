//must add easy nextion library

#include "EasyNextionLibrary.h"

EasyNex myNex(Serial1);

void setup() {

  Serial.begin(9600);

  myNex.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  myNex.NextionListen();

}

void trigger0() {
  Serial.println("hi");
}


/* old way of getting info from screen
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  String dfd = ""; //data from display

  while(Serial1.available() > 0) {
    dfd += int(Serial1.read());
  }

  Serial.println(dfd);
  delay(2000);
}
*/