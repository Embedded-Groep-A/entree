#include "WifiSocket.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Message.h"

// WiFi-instellingen
const char *ssid = "NSELab";               
const char *password = "NSELabWiFi";      
const char *host = "145.52.127.103";       
int port = 8181;                           

WiFiSocket wifiSocket;                     

// GPIO-pinnen voor output
#define MOTOR_PIN D1                       // Output pin voor het stoelmotor-signaal
#define SCHEMER_PIN D2                     // Output pin voor de schemerlamp

/**
 * @brief Setup-functie: initialiseert de seriële poort, pinnen en netwerkverbinding.
 */
void setup() {
  Serial.begin(115200);
  
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(SCHEMER_PIN, OUTPUT);

  digitalWrite(MOTOR_PIN, LOW);        // Motor standaard uit bij opstart
  digitalWrite(SCHEMER_PIN, LOW);      // Schemerlamp standaard uit bij opstart

  // Verbind met WiFi en socket-server
  wifiSocket.connectWiFi(ssid, password);
  wifiSocket.connectSocket(host, port, "WEMOSR");

  delay(1000); // korte wachttijd na verbinding
}

/**
 * @brief Pollt de server voor berichten en schakelt op basis van het bericht de relaxstoel aan of de schermerlamp.
 */
void loop() {
  auto [type, message] = wifiSocket.pollServer();  // Ontvang type en inhoud van het bericht

  if (type == MessageType::STATE) {
    if (message == "STOEL") {
      if (digitalRead(MOTOR_PIN) == HIGH) {
        digitalWrite(MOTOR_PIN, LOW);  // Zet motor uit
      } else {
        digitalWrite(MOTOR_PIN, HIGH); // Zet motor aan
      }
    }

    if (message == "SCHEMERLAMP") {
      if (digitalRead(SCHEMER_PIN) == HIGH) {
        digitalWrite(SCHEMER_PIN, LOW);  //Zet schemerlamp uit
      } else {
        digitalWrite(SCHEMER_PIN, HIGH); //Zet schemerlamp aan
      }
    }
  }
}
