#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h" // https://github.com/adafruit/Adafruit_MQTT_Library
#include "Timer.h"                // https://github.com/JChristensen/Timer/tree/v2.1
#include "Debounce.h"             // https://github.com/arnebech/Debounce

#define WLAN_SSID       "ENTER_SSID"
#define WLAN_PASS       "ENTER_SSID_PWD"

#define MQTT_SERVERPORT     1883
#define PING_TIME           60000   // 1 minute
#define CHECK_GARAGE_TIME   600000  // 10 minutes
#define LED_FLASH_TIME      200     // 0.2 seconds
#define LEDPIN        16        // LED Output pin
#define GRGE_1        12        // Garage Door - 1 input pin
#define GRGE_2        14        // Garage Door - 2 input pin
#define GRGE_3        13        // Garage Door - 3 input pin

const char MQTT_SERVER[] PROGMEM    = "ENTER_MQTT_IP";
const char MQTT_USERNAME[] PROGMEM  = "ENTER_MQTT_USR";
const char MQTT_PASSWORD[] PROGMEM  = "ENTER_MQTT_PWD";

String deviceName = "GARAGE";
char szBuffer[100] = "\0";
char szState[15] = "\0";
char szIteration[10] = "\0";
int garage1Iteration = 0;
int garage2Iteration = 0;
int garage3Iteration = 0;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);
Timer timer;
Debounce debounce = Debounce();

void setup() {
  //Serial.begin(115200);
  //delay(10);
  //Serial.println(F("Garage Door Sensor"));
  
  pinMode(LEDPIN, OUTPUT);
  pinMode(GRGE_1, INPUT_PULLUP);
  pinMode(GRGE_2, INPUT_PULLUP);
  pinMode(GRGE_3, INPUT_PULLUP);

  debounce.setBounceDelay(150); // 150 millSeconds debounce delay
  debounce.addInput(GRGE_1, INPUT_PULLUP, callbackGarage1);
  debounce.addInput(GRGE_2, INPUT_PULLUP, callbackGarage2);
  debounce.addInput(GRGE_3, INPUT_PULLUP, callbackGarage3);

  // Connect to WiFi access point.
  //Serial.println(); Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println();
  //Serial.println("WiFi connected");
  //Serial.println("IP address: "); 
  //Serial.println(WiFi.localIP());

  mqttConnect();
  
  timer.oscillate(LEDPIN, LED_FLASH_TIME, HIGH);
  timer.every(PING_TIME, pingMQTTMessage, (void*)0);
  timer.every(CHECK_GARAGE_TIME, callbakGarageDoorStatus, (void*)0);

  sprintf(szBuffer, "SENSOR/%s/STATUS", deviceName.c_str());
  sendMessage(szBuffer, "STARTED");
  //Serial.println(szBuffer);
}

void loop() {
  timer.update();
  debounce.update();
  mqttConnect();
}

void callbackGarage1(bool state, uint8_t pin) {
  //sprintf(szBuffer, "PORT:%u=%u", pin, state);
  //Serial.println(szBuffer);
  actionGarage(state, 81, 0);
  if(state == LOW) {
    garage1Iteration = 0;
  }
}

void callbackGarage2(bool state, uint8_t pin) {
  //sprintf(szBuffer, "PORT:%u=%u", pin, state);
  //Serial.println(szBuffer);
  actionGarage(state, 82, 0);
  if(state == LOW) {
    garage2Iteration = 0;
  }
}

void callbackGarage3(bool state, uint8_t pin) {
  //sprintf(szBuffer, "PORT:%u=%u", pin, state);
  //Serial.println(szBuffer);
  actionGarage(state, 83, 0);
  if(state == LOW) {
    garage3Iteration = 0;
  }
}

void callbakGarageDoorStatus(void *context) {
  if(digitalRead(GRGE_1) == HIGH) {
    actionGarage(HIGH, 81, ++garage1Iteration);
  }
  if(digitalRead(GRGE_2) == HIGH) {
    actionGarage(HIGH, 82, ++garage2Iteration);
  }
  if(digitalRead(GRGE_3) == HIGH) {
    actionGarage(HIGH, 83, ++garage3Iteration);
  }  
}

void actionGarage(bool state, uint8_t doorNum, int iteration) {
  sprintf(szBuffer, "SENSOR/%s/PORT/%u", deviceName.c_str(), doorNum);
  if(iteration > 0) {
    sprintf(szIteration, " [%d]", iteration);
  } else {
    sprintf(szIteration, "");
  }
  sprintf(szState, "%s%s", (state ? "Open" : "Close"), szIteration);
  sendMessage(szBuffer, szState);  
}

void pingMQTTMessage(void *context) {
  sprintf(szBuffer, "SENSOR/%s/STATUS", deviceName.c_str());
  sendMessage(szBuffer, "ACTIVE");
  //Serial.println(szBuffer);
}

void sendMessage(String topic, String message) {
  if (!mqtt.publish((char*) topic.c_str(), (char*) message.c_str())) {
    //Serial.println("Unable to send MQTT message!");
  }
}

void mqttConnect() {
  int8_t ret;
  if (mqtt.connected()) {
    return;
  }
  //Serial.print("Connecting to MQTT... ");
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       //Serial.println(mqtt.connectErrorString(ret));
       //Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  //Serial.println("MQTT Connected!");
}

