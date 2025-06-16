#include "WifiSocket.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
/*!
 * \file main.cpp
 * \brief Main sourcecode file of RGB and PIR sensor WEMOS
 *      Documentation:  https://embedded-groep-a.github.io/entree/
 */

/** @brief WiFi SSID voor netwerkverbinding */
const char *ssid = "NSELab";
/** @brief WiFi-wachtwoord */
const char *password = "NSELabWiFi";
/** @brief IP-adres van de server */
const char *host = "145.52.127.103";
/** @brief TCP-poort van de server */
int port = 8181;

/** @brief GPIO-pin voor de PIR sensor (D1) */
const int pirPin = 5;  

/** @brief GPIO-pin voor de rode LED (D6) */
const int redPin = 12;  

/** @brief GPIO-pin voor de groene LED (D7) */
const int greenPin = 14;  

/** @brief GPIO-pin voor de blauwe LED (D5) */
const int bluePin = 13;   

/** @brief Maakt een Instance van de WiFiSocket*/
WiFiSocket wifiSocket;

/**
 * @brief Zet de RGB LED op een specifieke kleur
 * 
 * @param r Roodwaarde (0–255)
 * @param g Groenwaarde (0–255)
 * @param b Blauwwaarde (0–255)
 */

void setRGB(int r, int g, int b) {
    analogWrite(redPin, r);
    analogWrite(greenPin, g);
    analogWrite(bluePin, b);
}

/**
 * @brief Initialiseert seriële communicatie, RGB pins en wifi + socketverbinding
 * 
 * Verbindt met het opgegeven WiFi-netwerk en opent een socket naar de server.
 */

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
/**
 * @brief Hoofdloop voor bewegingsdetectie en RGB-aansturing
 * 
 * Stuurt RGB waarden naar de LED bij beweging. Haalt RGB-commando’s van de server op.
 */

void loop() {
    /// Laatste ontvangen roodwaarde van de server
    static int lastR = 0, lastG = 0, lastB = 0;

    /// Houdt bij of de RGB LED momenteel aan staat
    static bool lightOn = false;

    bool motionDetected = digitalRead(pirPin);

    /**
 * @brief Checkt of de server een nieuw bericht heeft
 * 
 * Als het een RGB-bericht is, wordt de RGB-waarde opgeslagen voor gebruik bij beweging.
 */
    auto [type, message] = wifiSocket.pollServer();
    if (type == MessageType::RGB) {
        sscanf(message.c_str(), "%d %d %d", &lastR, &lastG, &lastB);
        Serial.printf("RGB van server: R=%d G=%d B=%d\n", lastR, lastG, lastB);
    }
/**
 * @brief Activeert RGB LED bij beweging
 * 
 * Als er beweging is gedetecteerd en het licht is nog uit:
 * - Zet RGB LED aan met laatst ontvangen waarden
 * - Stuurt status "BEWEGING" naar de server
 */
    if (motionDetected) {
        if (!lightOn) {
            Serial.println("Beweging gedetecteerd - RGB aan");
            setRGB(lastR, lastG, lastB);
            wifiSocket.sendToServer(MessageType::STATE, "BEWEGING");
            lightOn = true;
        }
    } else {
        /**
        * @brief Zet RGB LED uit als er geen beweging meer is
        */
        if (lightOn) {
            Serial.println("Geen beweging - RGB uit");
            setRGB(0, 0, 0);
            lightOn = false;
        }
    }

    delay(200);
}
