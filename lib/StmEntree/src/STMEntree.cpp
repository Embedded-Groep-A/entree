/**
 * @file STMEntree.cpp
 * @brief Implementatie van de STM32-gebaseerde entreecontroller met servoaandrijving en RS485.
 */

#include "STMEntree.h"
#include <PinNames.h>

// RS485 UART via PA_0 (TX), PA_1 (RX)
HardwareSerial RS485Serial(D0, D1);  // RX, TX

// STM32 pin definities
#define DEURBEL_PIN     D11
#define BINNENKANT_PIN  D9
#define NOODSTOP_PIN    D10
#define SERVOVOOR_PIN   PA_5
#define SERVOACHTER_PIN PA_4

Servo deurServovoor;
Servo deurServoAchter;

bool noodstopActief = false;
int lastNoodstopState = HIGH;

void initSTMEntree() {
  Serial.begin(115200);
  RS485Serial.begin(9600);

  pinMode(DEURBEL_PIN, INPUT_PULLUP);
  pinMode(BINNENKANT_PIN, INPUT_PULLUP);
  pinMode(NOODSTOP_PIN, INPUT_PULLUP);
  pinMode(SERVOVOOR_PIN, OUTPUT);
  pinMode(SERVOACHTER_PIN, OUTPUT);

  deurServovoor.attach(SERVOVOOR_PIN);
  deurServoAchter.attach(SERVOACHTER_PIN);

  deurServovoor.write(0);
  deurServoAchter.write(0);
}

void openDeur() {
  Serial.println("Deur openen...");
  deurServovoor.write(180);
  delay(2000);
  deurServovoor.write(0);
  delay(2000);
  deurServoAchter.write(180);
  delay(2000);
  deurServoAchter.write(0);
  delay(2000);
  Serial.println("Deur gesloten.");
}

void openDeurVanBinnen() {
  Serial.println("Achterdeur openen...");
  deurServoAchter.write(180);
  delay(2000);
  deurServoAchter.write(0);
  delay(2000);

  Serial.println("Voorste deur openen...");
  deurServovoor.write(180);
  delay(2000);
  deurServovoor.write(0);
  delay(2000);

  Serial.println("Beide deuren gesloten.");
}

void verwerkDeurbel() {
  Serial.println("Deurbel ingedrukt, stuur [BEL] naar Pi...");
  RS485Serial.println("[BEL] @");
}

void openDeurBrand() {
  deurServovoor.write(180);
  delay(2000);
  deurServoAchter.write(180);
}

void verwerkLoop() {
  // RS485 uitlezen per karakter, met lokale buffer
  static char rs485Buffer[64];
  static size_t idx = 0;

  while (RS485Serial.available()) {
    char c = RS485Serial.read();
    if (c == '\r' || c == '\n') {
      if (idx > 0) {
        rs485Buffer[idx] = '\0'; // afsluiten als C-string
        Serial.print("RS485 ontvangen: ");
        Serial.println(rs485Buffer);

        /**
         * @brief Zoek of de ontvangen RS485-string een bepaald commando bevat.
         *
         * De functie strstr() zoekt naar een substring binnen een C-string.
         * Bijvoorbeeld: strstr(rs485Buffer, "[OPEN]") geeft een pointer terug als de substring gevonden wordt,
         * of NULL als het niet voorkomt. Dit gebruiken we om te reageren op specifieke instructies zoals [OPEN] of [STATE] BRAND?.
         */
        if (strstr(rs485Buffer, "[OPEN]")) {
          openDeur();
        } 
        else if (strstr(rs485Buffer, "[STATE] BRAND?")) {
          openDeurBrand();
        }
        idx = 0; // buffer resetten
      }
    } else if (idx < sizeof(rs485Buffer) - 1) {
      rs485Buffer[idx++] = c;
    }
  }

  /**
   * @brief Detecteer de verandering in de noodstop-knop en activeer of deactiveer de noodmodus.
   *
   * De noodstop schakelt beide deuren volledig open bij activatie.
   * Bij deactiveren worden beide deuren weer gesloten.
   * De toestand wordt onthouden in `noodstopActief` en `lastNoodstopState` voorkomt bouncing-detectie.
   */
  int currentNoodstopState = digitalRead(NOODSTOP_PIN);
  if (lastNoodstopState == HIGH && currentNoodstopState == LOW) {
    if (!noodstopActief) {
      Serial.println("NOODSTOP ACTIEF!");
      deurServovoor.write(180);
      deurServoAchter.write(180);
      delay(2000);
      noodstopActief = true;
    } else {
      Serial.println("NOODSTOP opgeheven.");
      deurServovoor.write(0);
      deurServoAchter.write(0);
      delay(2000);
      noodstopActief = false;
    }
  }
  lastNoodstopState = currentNoodstopState;

  if (noodstopActief) {
    return;
  } 

  if (digitalRead(DEURBEL_PIN) == LOW) {
    verwerkDeurbel();
    delay(2000);
  }

  if (digitalRead(BINNENKANT_PIN) == LOW) {
    Serial.println("Binnenknop ingedrukt.");
    openDeurVanBinnen();
    delay(2000);
  }
}
