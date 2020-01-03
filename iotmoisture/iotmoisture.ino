/**
   An example showing how to put ESP8266 into Deep-sleep mode
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoUniqueID.h>
#include <ezTime.h>

// Variables - please change as needed
// ************************************************
#define WIFISID "mySID"
#define WIFIPW "myWIFIpw"
#define MQ_KEY "key"
#define MQ_SECRET "secret"
#define MQ_SRV "mqtt.example.com"
#define SLEEP_TIME 9e8
// ************************************************

// WiFi credentials.
const char* WIFI_SSID = WIFISID;
const char* WIFI_PASS = WIFIPW;

// MQTT information.
char* MQTT_DEVICE_ID;
const char* MQTT_ACCESS_KEY = MQ_KEY;
const char* MQTT_ACCESS_SECRET = MQ_SECRET;
const char* MQTT_SERVER = MQ_SRV;
const int   MQTT_SERVER_PORT = 1883;
const char* MQTT_TOPIC = "iot/sensors";

// Sensor data
int sensorValue = 0;  // value read from the pot
int outputValue = 0;  // value to output to a PWM pin
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0


WiFiClient wifiClient;

PubSubClient client(wifiClient);

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      delay(10000);
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.print("Connecting to MQTT...");
  client.setServer(MQTT_SERVER, MQTT_SERVER_PORT);

  while (!!!client.connected()) {
    Serial.print("Reconnecting client to ");
    Serial.print(MQTT_SERVER);
    Serial.print(" with DeviceID: ");
    Serial.println(MQTT_DEVICE_ID);
    while (!!!client.connect(MQTT_DEVICE_ID, MQTT_ACCESS_KEY, MQTT_ACCESS_SECRET)) {
      Serial.print(".");
      delay(2000);
    }
    Serial.println();
  }

  Serial.println("Connected!");
  Serial.println("This device is now ready for use!");
}

// For debug purposes or any other means
String getJSONPayload() {
  String payload = "{ \"sensor\": \"";
  payload += (char *)MQTT_DEVICE_ID;
  payload +="\". \"time\": \"";
  payload += UTC.dateTime(ISO8601);
  payload += "\", \"raw\": \"";
  payload += getMoisture();
  payload += "\", \"mapped \": \"";
  payload += outputValue;
  payload += "\"}";
  return payload;
}

String getInfluxPayload() {
  int moist = getMoisture();
  String payload = "moisture,sensor=";
  payload += (char *)MQTT_DEVICE_ID;
  payload += " humidity=";
  payload += outputValue;
  payload += ",rawdata=";
  payload += moist;
  payload += "";
  return payload;
}

void sendSensorData() {
  String payload = getInfluxPayload(); //getJSONPayload();
  
  Serial.print("Sending payload: ");
  Serial.println(payload);

  if (client.publish(MQTT_TOPIC, (char *)payload.c_str())) {
    Serial.println("Publish ok");
    client.disconnect();
  } else {
    Serial.println("Publish failed");
    if (!!!client.connected()) {
      Serial.print("Reconnecting client to ");
      Serial.println(MQTT_SERVER);
      while (!!!client.connect(MQTT_DEVICE_ID, MQTT_ACCESS_KEY, MQTT_ACCESS_SECRET)) {
        Serial.print(".");
        delay(2000);
      }
      Serial.println();
    }
  }
}

int getMoisture() {

  // read the analog in value
  sensorValue = analogRead(analogInPin);
  
  // map it to the range of the PWM out
  // moisture sensor reads approx 370 in a glass of water and abt. 800 in dry. We map these values to 0-100% humidity.
  outputValue = map(sensorValue, 370, 805, 100, 0);
  
  // print the readings in the Serial Monitor
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("\t output = ");
  Serial.println(outputValue);

  return(sensorValue);
}

void setup() {

  // Nasty way to get a string representation of the MAC address.
  // Is there a more elegant way?
  
  char macStr[20], idStr[20];
  WiFi.macAddress().toCharArray(macStr, 20);
  sprintf(idStr, "ESP-%s", macStr);
  MQTT_DEVICE_ID = idStr;

  Serial.begin(115200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while (!Serial) { }

  Serial.println("Moisture Sensor Device Started");
  Serial.println(MQTT_DEVICE_ID);
  Serial.println("-------------------------------------");
  Serial.println("Deep-Sleep Mode");
  Serial.println(" ------------------------------------");

  connect();
  Serial.println("Wating for time sync");
  waitForSync(60); // wait for time sync

  sendSensorData();
  long sleepTime = SLEEP_TIME;
  if(sleepTime > ESP.deepSleepMax())
    sleepTime = 9e8;

  Serial.print("Going into deep sleep for ");
  Serial.print(sleepTime);
  Serial.println(" micoseconds");
  ESP.deepSleep(sleepTime); // 20e6 is 20 seconds, 36e9 is 1hr
}

void loop() {
}
