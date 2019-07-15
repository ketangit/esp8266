#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MQTT_Client.h>
#include <Debounce.h> // https://github.com/arnebech/Debounce (copied in lib folder)
#include <Timer.h>    // https://github.com/JChristensen/Timer/tree/v2.1 (copied in lib folder)

#define WIFI_SSID "" // "ENTER_SSID"
#define WIFI_PASS "" // "ENTER_SSID_PWD"

String deviceName = "GARAGE-1";
const char MQTT_SERVER[] PROGMEM = "";   // "ENTER_MQTT_IP";
const char MQTT_USERNAME[] PROGMEM = ""; // "ENTER_MQTT_USR";
const char MQTT_PASSWORD[] PROGMEM = ""; // "ENTER_MQTT_PWD";

#define MQTT_PORT 1883
#define GRGE_1_PIN 12 // Garage Door - 1 input pin
#define GRGE_2_PIN 14 // Garage Door - 2 input pin
#define GRGE_3_PIN 13 // Garage Door - 3 input pin

//#define BUILTIN_LED_PIN 2  // DEMO
#define STATUS_LED 16        // Status LED Output pin
#define LED_FLASH_TIME 400   // 0.4 seconds
#define MQTT_PING_TIME 60000 // 1 minute

#define GRGE_LED_3 0           // Garage Door - 3 LED
#define GRGE_LED_2 1           // Garage Door - 2 LED
#define GRGE_LED_1 2           // Garage Door - 1 LED
#define GDOOR_OPEN_TIME 300000 // 5 minutes

#define PIN 2 // WS Led pin
#define NUMPIXELS 3

char szBuffer[100] = "\0";
char szState[30] = "\0";
int8_t doorOpenTimerArray[3] = {0};
int doorOpenTotalTime[3] = {0};

Debounce debounce = Debounce();
Timer timer;
WiFiClient client;
Adafruit_MQTT_Client mqttClient(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void pingMQTTMessage(void *context);
void sendMessage(String topic, String message);
void callbackGarage(bool state, uint8_t pin);
void callbackDoorOpen(void *context);
void mqttConnect();

void setup()
{
  //Serial.begin(115200);
  //while (!Serial)
  //  ; // wait for serial attach
  //Serial.println("Initializing...");
  //Serial.flush();

  pinMode(STATUS_LED, OUTPUT);
  pinMode(GRGE_1_PIN, INPUT_PULLUP);
  pinMode(GRGE_2_PIN, INPUT_PULLUP);
  pinMode(GRGE_3_PIN, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  timer.oscillate(STATUS_LED, LED_FLASH_TIME, HIGH);
  timer.every(MQTT_PING_TIME, pingMQTTMessage, (void *)0);

  strip.begin();
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.setPixelColor(1, strip.Color(0, 0, 0));
  strip.setPixelColor(2, strip.Color(0, 0, 0));

  callbackGarage(digitalRead(GRGE_1_PIN), GRGE_1_PIN);
  callbackGarage(digitalRead(GRGE_2_PIN), GRGE_2_PIN);
  callbackGarage(digitalRead(GRGE_3_PIN), GRGE_3_PIN);

  debounce.setBounceDelay(150); // 150 millSeconds debounce delay
  debounce.addInput(GRGE_1_PIN, INPUT_PULLUP, callbackGarage);
  debounce.addInput(GRGE_2_PIN, INPUT_PULLUP, callbackGarage);
  debounce.addInput(GRGE_3_PIN, INPUT_PULLUP, callbackGarage);
}

void loop()
{
  mqttConnect();
  debounce.update();
  timer.update();
  strip.show();
}

void pingMQTTMessage(void *context)
{
  sprintf(szBuffer, "SENSOR/%s/STATUS", deviceName.c_str());
  sendMessage(szBuffer, "ACTIVE");
}

void sendMessage(String topic, String message)
{
  mqttClient.publish((char *)topic.c_str(), (char *)message.c_str());
}

void callbackGarage(bool state, uint8_t pin)
{
  uint8_t ledNum = 0;
  uint8_t doorNum = 0;
  if (pin == GRGE_1_PIN)
  {
    doorNum = 81;
    ledNum = GRGE_LED_1; // 2
  }
  else if (pin == GRGE_2_PIN)
  {
    doorNum = 82;
    ledNum = GRGE_LED_2; // 1
  }
  else if (pin == GRGE_3_PIN)
  {
    doorNum = 83;
    ledNum = GRGE_LED_3; // 0
  }
  if (state)
  {
    strip.setPixelColor(ledNum, strip.Color(150, 0, 0));
    doorOpenTimerArray[ledNum] = timer.every(GDOOR_OPEN_TIME, callbackDoorOpen, (void *)pin);
  }
  else
  {
    strip.setPixelColor(ledNum, strip.Color(0, 150, 0));
    doorOpenTotalTime[ledNum] = 0;
    timer.stop(doorOpenTimerArray[ledNum]);
  }
  strip.show();
  sprintf(szBuffer, "SENSOR/%s/PORT/%u", deviceName.c_str(), doorNum);
  sprintf(szState, "%s", (state ? "Open" : "Close"));
  sendMessage(szBuffer, szState);
}

void callbackDoorOpen(void *context)
{
  int pin = int(context);
  uint8_t ledNum = 0;
  uint8_t doorNum = 0;
  if (pin == GRGE_1_PIN)
  {
    doorNum = 81;
    ledNum = GRGE_LED_1; // 2
  }
  else if (pin == GRGE_2_PIN)
  {
    doorNum = 82;
    ledNum = GRGE_LED_2; // 1
  }
  else if (pin == GRGE_3_PIN)
  {
    doorNum = 83;
    ledNum = GRGE_LED_3; // 0
  }
  doorOpenTotalTime[ledNum] = doorOpenTotalTime[ledNum] + (GDOOR_OPEN_TIME / 60000);
  sprintf(szBuffer, "ALERT/%s/PORT/%u", deviceName.c_str(), doorNum);
  sprintf(szState, "Open for %u minutes", doorOpenTotalTime[ledNum]);
  sendMessage(szBuffer, szState);
}

void mqttConnect()
{
  int8_t ret;
  if (mqttClient.connected())
  {
    return;
  }
  while ((ret = mqttClient.connect()) != 0)
  { // connect will return 0 for connected
    mqttClient.disconnect();
    delay(5000); // wait 5 seconds
  }
}