#include <Arduino.h>

#define BUILTIN_LED1 2

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");
  pinMode(BUILTIN_LED1, OUTPUT);
}

void loop()
{
  digitalWrite(BUILTIN_LED1, LOW);
  Serial.println("Led is off");
  delay(100);
  digitalWrite(BUILTIN_LED1, HIGH);
  Serial.println("Led is on");
  delay(1000);
}
