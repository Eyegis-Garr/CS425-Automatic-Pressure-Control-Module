#include "EasyNextionLibrary.h"

//using test gui

EasyNex myNex(Serial3);

void setup() {
  // put your setup code here, to run once:
  Serial3.begin(9600);
  Serial.begin(9600);
  myNex.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  myNex.NextionListen();
}

void trigger0() {
  String str = myNex.readStr("page1.t1.txt");
  if(str.endsWith("|")) {
    str.remove(str.length() -1);
    Serial.println(str);
  }
  else {
    Serial.println(str);
  }
}