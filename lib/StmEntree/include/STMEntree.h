/**
 * @file STMEntree.h
 * @brief Header voor de servo-aansturing van de luchtsluis met noodstop en RS485-communicatie.
 */

#pragma once
#include <Arduino.h>
#include <Servo.h>

/**
 * @brief Initialiseert pinnen, servomotoren en seriële communicatie.
 */
void initSTMEntree();

/**
 * @brief Opent de deuren van buitenaf (voorste deur, daarna achterste deur).
 */
void openDeur();

/**
 * @brief Opent de deuren van binnenuit (achterdeur eerst, dan voordeur).
 */
void openDeurVanBinnen();

/**
 * @brief Stuurt een bericht naar de Pi wanneer de deurbel wordt ingedrukt.
 */
void verwerkDeurbel();

/**
 * @brief Opent beide deuren direct bij brandmelding (blijft open).
 */
void openDeurBrand();

/**
 * @brief Verwerkt RS485-invoer, knoppen en noodstop. Aanroepen vanuit loop().
 */
void verwerkLoop();
