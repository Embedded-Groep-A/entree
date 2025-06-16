/**
 * @file STMEntree.h
 * @brief Header voor de servo-aansturing van de luchtsluis met noodstop en RS485-communicatie.
 */

#pragma once
#include <Arduino.h>
#include <Servo.h>


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

