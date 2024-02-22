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