#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "button.h"
#include "pin.h"
// 1 led, 1 button
#define LED 0
#define BUTTON 2
#define DEBUG_MODE  1               //1 set debug on

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
char device_id[6] = "1";
char device_name[40] = "esp8266";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

Button button(BUTTON);
Pin led(LED);

WiFiClient espClient;
PubSubClient client(espClient);

// Functions
void callback(char* topic, byte* payload, unsigned int length) {
  /* rest.handle_callback(client, topic, payload, length); */
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
		// but actually the LED is on; this is because
		// it is acive low on the ESP-01)

  } else {
    digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

const char* publish_channel(){
  return "pomodoro_clock/1/out";
}

const char* subscribe_channel(){
  return "pomodoro_clock/1/in";
}

void setup() {
  pinMode(LED, OUTPUT);
  button.setup();

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //read configuration from FS json
  Serial.println("mounting FS...");
  delay(2000);
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
	Serial.println("config.json");
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(device_id, json["device_id"]);
          strcpy(device_name, json["device_name"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_device_id("device_id", "device id", device_id, 6);
  WiFiManagerParameter custom_device_name("device_name", "device name", device_name, 40);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_device_id);
  wifiManager.addParameter(&custom_device_name);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_id, custom_device_id.getValue());
  strcpy(device_name, custom_device_name.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["device_id"] = device_id;
    json["device_name"] = device_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  // --------------
  Serial.print("mqtt_server: ");
  Serial.println(mqtt_server);
  Serial.print("port: ");
  Serial.println(atoi(mqtt_port));
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  // --------------
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
      client.publish("pomodoro_clock/1/out", "connected");

      Serial.println("publish to: ");
      Serial.println(publish_channel());
      // ... and resubscribe
      client.subscribe(subscribe_channel());
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

  if (led.is_changed(digitalRead(led.pin))) {
    client.publish(publish_channel(), "changed");
  }

  if (button.low()) {
    client.publish(publish_channel(), "button low");
  }
}
