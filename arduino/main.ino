
#include "ESP8266WiFi.h"
#include "secrets.h" //NOTE! File needs to be added and contain WIFI_PASSWORD

#include <SoftwareSerial.h>
SoftwareSerial ESPserial(2, 3); // RX | TX

const char* ssid     = "comhem_D4737F";
const char* password = WIFI_PASSWORD;

const char* host = "192.168.0.204";
const int httpPort = 8186;
const String uri = "/write?db=influx&precision=ms";

// Pin for vibration module (GPIO2 pin on ESP8266-1)
const int shockPin = 2;

// Built in led pin
const int ledPin = 13;

// Debounce millis tries to ensure that we dont register
// one vibration as muiltiple. Value needs to at least be above 200ms but 
// depends on how much the component can move.
const int debounceMs = 350;

// As we have no other way to register when lap started than when previous
// lap ended we set a timeout for laps. This will also be what gets registered the "first" laps.
const int lapTimeoutMs = 2500;

unsigned long lastLapTimestampMs = 0;

int shockValue = 0;

void setup() {
  // Start the software serial for communication with the ESP8266
  Serial.begin(9600);
  ESPserial.begin(115200);
  ESPserial.println("AT+IPR=9600");

  delay(1000);
  
  ESPserial.end();
  ESPserial.begin(9600);

  Serial.println("Ready");
  ESPserial.println("AT+GMR");

  delay(5000);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set ESP8266 to wifi-client mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Listen for vibration 
  pinMode(shockPin, INPUT);
}

void loop() {
  shockValue = digitalRead(shockPin);

  if (shockValue == LOW) {
    int msSinceLastLap = millis() - lastLapTimestampMs;
    if (msSinceLastLap < debounceMs) {
        // Ignore lap, there is no way poppe did a lap that fast
    } else { 
        lastLapTimestampMs = millis();
        int lapTimeMs = msSinceLastLap<lapTimeoutMs?msSinceLastLap:lapTimeoutMs;
        reportLap(lapTimeMs);
    }
  }
}

void reportLap(int lapTime) {
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed, make sure telegraf is running with http listener on port 8186.");
    return;
  }

  // Needs to comply with influx line protocal: https://docs.influxdata.com/influxdb/v1.8/write_protocols/line_protocol_tutorial/
  // As we dont care about any response here it will fail silently.
  String data = "hamster,host=poppe lapTime=" + String(lapTime) + ",lapCount=1";

  client.println(String("POST ") + uri + " HTTP/1.1");
  client.println(String("Host: ") + host);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.println(data);

  Serial.print("Request data: ");
  Serial.println(data);
            
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout!");
      client.stop();
      return;
    }
  }
}