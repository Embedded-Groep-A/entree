/**
 * @file RFID.cpp
 * @brief RFID-kaartlezer met Wemos D1 Mini en WiFi communicatie naar Raspberry Pi.
 */
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Message.h"
#include "WifiSocket.h"

// WiFi gegevens
const char *ssid = "NSELab";               
const char *password = "NSELabWiFi";       
const char *host = "145.52.127.103";      
const int port = 8181;                     

WiFiSocket wifiSocket;                     
/// RFID pinnen op de Wemos D1 mini
#define RST_PIN D3                         // Reset-pin van de MFRC522-module
#define SS_PIN  D4                         // Slave Select-pin van de MFRC522-module

MFRC522 mfrc522(SS_PIN, RST_PIN);          

/**
 * @brief Initialisatie van seriële communicatie, RFID-module en WiFi-verbinding.
 */
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Wemos RFID gestart");

  SPI.begin();              
  mfrc522.PCD_Init();      
  Serial.println("RFID klaar, scan een kaart...");

  // Verbinden met WiFi en socket naar server
  wifiSocket.connectWiFi(ssid, password);
  wifiSocket.connectSocket(host, port, "WEMOSUID");
}

/**
 * @brief Loop-functie die controleert op RFID-kaarten en de UID naar de server stuurt.
 */
void loop() {
  // Controleer of er een nieuwe kaart is en lees deze
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // UID omzetten naar stringformaat
  char uidString[64];  
  uidString[0] = '\0'; 

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    char byteStr[6];  
    if (i < mfrc522.uid.size - 1)
      snprintf(byteStr, sizeof(byteStr), "%02X ", mfrc522.uid.uidByte[i]);
    else
      snprintf(byteStr, sizeof(byteStr), "%02X", mfrc522.uid.uidByte[i]);

    /**
     * @brief Voeg de byte als hexadecimale string toe aan uidString.
     * 
     * strncat zorgt ervoor dat we telkens één byte toevoegen aan het eind van uidString,
     * zonder dat we buiten de buffergrenzen gaan (de maximale lengte wordt gerespecteerd).
     */
    strncat(uidString, byteStr, sizeof(uidString) - strlen(uidString) - 1);
  }

  Serial.print("UID: ");
  Serial.println(uidString);

  // Verstuur de UID naar de server
  wifiSocket.sendToServer(MessageType::UID, String(uidString));

  // Stop met lezen van de huidige kaart
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1000);  // kleine pauze om dubbele scans te voorkomen
}
