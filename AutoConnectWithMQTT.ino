#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <Button.h>               //https://github.com/r89m/PushButton
#include <ButtonEventCallback.h>
#include <PushButton.h>

#include "device.h"
#include "config.h"
#include <string.h>

#define DEBUG_MODE  1               //1 set debug on

Device device;

void callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);

// Cuando llega un mensage por mqtt se imprime en el serial
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived:");
  char message = payload[0];
  Serial.print(message);
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  setup_config();
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  
  device.set(device_name, device_id);
  Serial.println("setup ended");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(device.publish_channel(), "connected");

      Serial.println("publish to: ");
      Serial.println(device.publish_channel());
      // ... and resubscribe
      Serial.println("subcribe to: ");
      Serial.println(device.subscribe_channel());
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

  client.loop();

  // Publica por mqtt el mensaje que viene por serial
  if (Serial.available() > 0) {
    client.publish(device.publish_channel(), Serial.readString().c_str());
  }
}
