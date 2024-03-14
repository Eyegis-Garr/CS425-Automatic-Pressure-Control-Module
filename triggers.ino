void trigger1() {     // CIRCUIT SELECT TRIGGER
  uint8_t sel = sys.ui.nex->readNumber("s");

  Serial.print("Circuit Select Trigger ");
  Serial.println(sel);

  sys.ui.cmask |= (1 << sel);
}

void trigger2() {     // PARAMETER SELECT TRIGGER
  uint8_t sel = sys.ui.nex->readNumber("s");

  Serial.print("Parameter Select Trigger ");
  Serial.println(sel);

  sys.ui.pmask |= (1 << sel);
}

void trigger3() {     // VALUE SET TRIGGER
  String valstr;
  double value = -1;
  int i, k;

  valstr = sys.ui.nex->readStr("Enter_Numbers.t1.txt");

  Serial.print("Value Set Trigger ");
  Serial.println(valstr);

  sscanf(valstr.c_str(), "%lf", &value);

  Serial.print("Scanned value ");
  Serial.println(value);

  for (i = 0; i < C_NUM_CIRCUITS; i += 1) {
    if (isbset(sys.ui.cmask, i)) {
      for (k = 0; k < C_NUM_PARAM; k += 1) {
        if (isbset(sys.ui.pmask, k)) {
          sys.circuits[i].params[k] = value;
        }
      }
    }
  }
}
