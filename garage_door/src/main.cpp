#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <NeoPixelBus.h>
#include <Debounce.h> // https://github.com/arnebech/Debounce (copied in lib folder)
#include <Timer.h>    // https://github.com/JChristensen/Timer/tree/v2.1 (copied in lib folder)

/*
  To Program, connect the borad TX->RX, RX->TX, VCC->VCC, GND->GND
   Press both (Reset & Flash) buttons at the sametime,
   then Release Reset first and then Flash button
   then Upload the this program
   https://arduino-esp8266.readthedocs.io/
*/

#define WIFI_SSID "ENTER_SSID"            // "ENTER_SSID"
#define WIFI_PASS "ENTER_SSID_PWD"        // "ENTER_SSID_PWD"
#define MQTT_HOST IPAddress(127, 0, 0, 1) // "ENTER_MQTT_IP"
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

char szBuffer[100] = "\0";
char szState[30] = "\0";
long lastReconnectAttempt = 0;
int8_t doorOpenTimerArray[3] = {0};
long doorOpenTotalTime[3] = {0};
String deviceName = "GARAGE-1";

Debounce debounce = Debounce();
Timer timer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

#define colorSaturation 128
const uint16_t PixelCount = 3; // number of NeoPixels (one for each garage door)
const uint8_t PixelPin = 2;    // ignored for Esp8266 default GPIO2
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> strip(PixelCount, PixelPin);
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);

void pingMQTTMessage(void *context);
void sendMessage(String topic, String message);
void callbackGarage(bool state, uint8_t pin);
void callbackDoorOpen(void *context);
void connectToWifi();
void onWifiConnect(const WiFiEventStationModeGotIP &event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);

void setup()
{
  //Serial.begin(115200);
  //while (!Serial)
  //  ; // wait for serial attach
  //Serial.println("Initializing...");
  //Serial.flush();

  pinMode(STATUS_LED, OUTPUT);
  timer.oscillate(STATUS_LED, LED_FLASH_TIME, HIGH);
  timer.every(MQTT_PING_TIME, pingMQTTMessage, (void *)0);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  strip.Begin();
  strip.Show();

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
  debounce.update();
  timer.update();
}

void pingMQTTMessage(void *context)
{
  sprintf(szBuffer, "SENSOR/%s/STATUS", deviceName.c_str());
  sendMessage(szBuffer, "ACTIVE");
}

void sendMessage(String topic, String message)
{
  mqttClient.publish((char *)topic.c_str(), 1, true, (char *)message.c_str());
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
    strip.SetPixelColor(ledNum, red);
    doorOpenTimerArray[ledNum] = timer.every(GDOOR_OPEN_TIME, callbackDoorOpen, (void *)pin);
  }
  else
  {
    strip.SetPixelColor(ledNum, green);
    doorOpenTotalTime[ledNum] = 0;
    timer.stop(doorOpenTimerArray[ledNum]);
  }
  strip.Show();
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

/*
  Wifi and MQTT functions
 */
void connectToWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt()
{
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  sprintf(szBuffer, "SENSOR/%s/STATUS", deviceName.c_str());
  sendMessage(szBuffer, "STARTED");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}
