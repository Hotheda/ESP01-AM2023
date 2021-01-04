#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <AM2320.h>
#include "credentials.h"

ADC_MODE(ADC_VCC);

#define MQTT_VERSION MQTT_VERSION_3_1_1

// MQTT: topic
const PROGMEM char* MQTT_SENSOR_TOPIC = "home/outside/sensor";
//const PROGMEM char* MQTT_SENSOR_TOPIC = "home/hallway/sensor";

// sleeping time
const PROGMEM uint16_t SLEEPING_TIME_IN_SECONDS = 600; // 60 seconds

// AM2320 Gpio0 and Gpio2
AM2320 am2320sensor;


WiFiClient wifiClient;
PubSubClient client(wifiClient);

float voltage;
float temp;
float humid;

int getVoltagePercent(){
  // from 2.5v to 3.4v
  float v = voltage - 2.5;
  if ( v < 0 ){
    v = 0;
  }
  return ( (v / 0.9 * 100) );
}


// function called to publish the temperature and the humidity
void publishData() {
  // create a JSON object
  StaticJsonDocument<200> jsonDoc;
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  jsonDoc["temperature"] = (String)temp;
  jsonDoc["humidity"] = (String)humid;
  jsonDoc["voltage"] = (String)getVoltagePercent();
  
  char data[200];
  serializeJson(jsonDoc, data);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}
// function called to publish ERROR
void publishData(String error) {
  // create a JSON object
  StaticJsonDocument<200> jsonDoc;
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  jsonDoc["error"] = error;

  char data[200];
  serializeJson(jsonDoc, data);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      //Serial.println("INFO: connected");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  //Serial.begin(115200);
  voltage = ((float) ((int)(ESP.getVcc() / 10.24 )) / 100 );
  pinMode(3, OUTPUT);

  digitalWrite(3, HIGH);
  delay(1000);
  
  //AM2320 Sensor setup
  Wire.begin(0,2); //IC2 SDA, SCL pins
  am2320sensor.begin();
  
  if(am2320sensor.measure()) {
    temp = am2320sensor.getTemperature();
    humid = am2320sensor.getHumidity();    
  } else {
    temp = 99.0f;
    humid = 99.0f;
  }

  digitalWrite(3, LOW);

  // init the WiFi connection
  WiFi.mode(WIFI_STA);
  //Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  
  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  publishData();

  //Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  //Serial.println("INFO: Closing the Wifi connection");
  WiFi.disconnect();

  
  ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1500000, WAKE_RF_DEFAULT);
  delay(500); // wait for deep sleep to happen
}
