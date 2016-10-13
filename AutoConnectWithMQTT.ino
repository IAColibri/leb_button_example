#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>

#include "pin.h"
#include "device.h"
#include "config.h"
// 1 led, 1 button
#define LED 0
#define BUTTON 2
#define DEBUG_MODE  1               //1 set debug on

PushButton button = PushButton(BUTTON);
Pin led(LED);
Device device;

WiFiClient espClient;
PubSubClient client(espClient);

void led_to(int pin, int value){
  digitalWrite(pin, value);
  client.publish(device.publish_channel(), "change");
}

// Functions
void callback(char* topic, byte* payload, unsigned int length) {
  /* rest.handle_callback(client, topic, payload, length); */
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    led_to(LED, LOW);
  } else {
    led_to(LED, HIGH);
  }
}

//callbacks functions
void pressFunc(Button& btn){
  client.publish(device.publish_channel(), "Pressed");
}

void setup() {
  pinMode(LED, OUTPUT);

  button.onPress(pressFunc);

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  setup_config();
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  // --------------

  device.set(device_name, device_id);
  Serial.println("setup ended");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(device.publish_channel(), "connected");

      Serial.println("publish to: ");
      Serial.println(device.publish_channel());
      // ... and resubscribe
      client.subscribe(device.subscribe_channel());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
 if (!client.connected()) {
    Serial.println("re connecting");
    reconnect();
  }

  button.update();
}
