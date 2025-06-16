#include "WifiSocket.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

const char *ssid = "NSELab";
const char *password = "NSELabWiFi";
const char *host = "145.52.127.103";
int port = 8181;

const int pirPin = 5;     // D1
const int redPin = 12;    // D6
const int greenPin = 14;  // D7
const int bluePin = 13;   // D5

WiFiSocket wifiSocket;

void setRGB(int r, int g, int b) {
    analogWrite(redPin, r);
    analogWrite(greenPin, g);
    analogWrite(bluePin, b);
}

void setup() {
    Serial.begin(115200);
    pinMode(pirPin, INPUT);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);

    setRGB(0, 0, 0);  // RGB uit bij start

    wifiSocket.connectWiFi(ssid, password);
    wifiSocket.connectSocket(host, port, "WEMOSRGB");
    delay(1000);
}

void loop() {
    static int lastR = 0, lastG = 0, lastB = 0;
    static bool lightOn = false;

    bool motionDetected = digitalRead(pirPin);

    // Altijd server checken
    auto [type, message] = wifiSocket.pollServer();
    if (type == MessageType::RGB) {
        sscanf(message.c_str(), "%d %d %d", &lastR, &lastG, &lastB);
        Serial.printf("RGB van server: R=%d G=%d B=%d\n", lastR, lastG, lastB);
    }

    if (motionDetected) {
        if (!lightOn) {
            Serial.println("Beweging gedetecteerd - RGB aan");
            setRGB(lastR, lastG, lastB);
            wifiSocket.sendToServer(MessageType::STATE, "BEWEGING");
            lightOn = true;
        }
    } else {
        if (lightOn) {
            Serial.println("Geen beweging - RGB uit");
            setRGB(0, 0, 0);
            lightOn = false;
        }
    }

    delay(200);
}
