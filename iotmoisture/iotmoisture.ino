/******************************************************************
 * Matthias Kubik
 * See project details at https://github.com/mkubik/IoTMoisture
 * 
 * 
 * MIT License
 * 
 * Copyright (c) 2019 mkubik
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************/
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266TrueRandom.h>

#define HTTP_PORT 80


int sensorValue = 0;  // value read from the pot
int outputValue = 0;  // value to output to a PWM pin
byte uuidNumber[16]; // UUIDs in binary form are 16 bytes long
String uuidStr;

const int A0Pin = A0;  // ESP8266 Analog Pin ADC0 = A0
const char* ssid = "<your SSID>";
const char* passwd = "<your wifi password>";

ESP8266WebServer http_server(HTTP_PORT);

int start_wifi() {
  int retries = 0;

  Serial.println("Connecting to WiFi AP..........");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passwd);
  // check the status of WiFi connection to be WL_CONNECTED
  while ((WiFi.status() != WL_CONNECTED) && (retries < 20)) {
    retries++;
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status(); // return the WiFi connection status
}

void get_moisture() {

  // read the analog in value
  sensorValue = analogRead(A0Pin);
  
  // moisture sensor reads approx 370 in a glass of water and abt. 800 in dry. We map these values to approx. 0-100% humidity.
  outputValue = map(sensorValue, 370, 805, 100, 0);
  
  // print the readings in the Serial Monitor
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("\t output = ");
  Serial.println(outputValue);

  // return a Dasheroo compatible JSON document. See https://www.dasheroo.com
  // If you use some other KPI dashboard, you may need to change the return document.
  StaticJsonDocument<512> doc;
  char buf[512];
  StaticJsonDocument<127> sensor;
  sensor["type"] = "integer";
  sensor["strategy"] = "continous";
  sensor["value"] = sensorValue;
  sensor["label"] = uuidStr;
  sensor["order"] = 0;
  StaticJsonDocument<127> moisture;
  moisture["type"] = "integer";
  moisture["strategy"] = "continous";
  moisture["value"] = outputValue;
  moisture["label"] = uuidStr;
  moisture["order"] = 1;
  doc["sensor"] = sensor;
  doc["moisture"] = moisture;
  serializeJsonPretty(doc, buf);
  http_server.send(200, "application/json", buf);
}

void init_server_routing() {
    http_server.on("/", HTTP_GET, []() {
        http_server.send(200, "text/html", "IoT Moisture Sensor Server\n");
    });
    http_server.on("/moisture", HTTP_GET, get_moisture);
}

void setup() {
  // initialize serial communication at 115200
  Serial.begin(115200);
  if (start_wifi() == WL_CONNECTED) {
    Serial.print("Connected to ");
    Serial.print(ssid);
    Serial.print(" with IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.print("Error connecting to: ");
    Serial.println(ssid);
  }
  // Generate a UUID for unique identification
  // Note that this is re-generated on each reboot/reset.
  // It is just intended to be able to distinguish if you have more of those sensors
  ESP8266TrueRandom.uuid(uuidNumber);
  uuidStr = ESP8266TrueRandom.uuidToString(uuidNumber);
  Serial.print("UUID: ");
  Serial.println(uuidStr);

  init_server_routing();
  http_server.begin();

}

void loop() {
  http_server.handleClient();
}
