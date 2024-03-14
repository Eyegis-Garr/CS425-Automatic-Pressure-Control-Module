//Vladislav Petrov
//Event Format Unit Test

#include <AUnit.h>
#include <AUnitVerbose.h>

String logEventFormat(unsigned long, String, String);

test(logEventFormat)
{
  unsigned long testTime = 12345;
  String eventType = "Event";
  String eventName = "Name";
  String expected = "12345: Event, Name";

  String result = logEventFormat(testTime, eventType, eventName);

  assertEqual(result, expected);
}

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  aunit::TestRunner::run();
  return(0);
}

String logEventFormat(unsigned long eventTime, String eventType, String eventName)
{
  String formattedTime = String(eventTime);
  return(String(formattedTime + ": " + eventType + ", " + eventName));
}
