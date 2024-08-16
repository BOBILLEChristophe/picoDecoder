/*

   picoDecoder

*/

#include <Arduino.h>
#include "SatellitePico.h"

#define PROJECT "picoDecoder"
#define VERSION "0.4.0"
#define AUTHOR "Christophe BOBILLE - www.locoduino.org"

//----------------------------------------------------------------------------------------
//  Board Check
//----------------------------------------------------------------------------------------

#ifndef ARDUINO_ARCH_RP2040
#error "Select a Raspberry Pi Pico board"
#endif

#if F_CPU != 128000000L
#error "CPU frequency is wrong. It should be set to 128 MHz"
#endif

const uint16_t thisHash = 0x00; // Identifiant unique du module (UID)
const uint8_t nbSensors = 16;   // Choisir le nombre d'entrées souhaitées
const byte sensorInPin[nbSensors] = {3, 4, 5, 6, 7, 8, 14, 15, 16, 17, 18, 19, 20, 21, 27, 28};

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  while (!Serial)
  {
    delay(50);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  Serial.printf("\nProject   :    %s", PROJECT);
  Serial.printf("\nVersion   :    %s", VERSION);
  Serial.printf("\nAuteur    :    %s", AUTHOR);
  Serial.printf("\nFichier   :    %s", __FILE__);
  Serial.printf("\nCompiled  :    %s", __DATE__);
  Serial.printf(" - %s\n\n", __TIME__);
  Serial.printf("-----------------------------------\n\n");

  Serial.println("Start setup");

  ACAN2515Settings settings(k2515ClockFrequency, 250UL * 1000UL);

  const uint16_t errorCode = gSat.begin(settings);

  if (errorCode == 0)
    Serial.print("Configuration CAN ok.");
  else
  {
    Serial.print("Configuration CAN error 0x");
    Serial.println(errorCode, HEX);
    printACAN2515Error(Serial, errorCode);
    return;
  }

  //--- init des broches des capteurs
  for (int i = 0; i < nbSensors; i++)
    pinMode(sensorInPin[i], INPUT_PULLUP);
}

CANMessage frame;

// ——————————————————————————————————————————————————————————————————————————————
//    LOOP
// ——————————————————————————————————————————————————————————————————————————————

void loop()
{
  uint16_t state = 0;
  for (byte i = 0; i < nbSensors; i++)
  {
    if (!digitalRead(sensorInPin[i])) // MAJ des capteurs
      state |= 1 << i;
  }
  frame.data16[0] = state;
  frame.id = thisHash;
  frame.ext = 0;
  frame.len = 2;
  const bool ok = gSat.can.tryToSend(frame);
  if (ok)
    Serial.printf("Sent: %d\n", frame.data16[0]);
  else
    Serial.println("Send failure\n");
  delay(100); // Toutes les 100ms
}

// ——————————————————————————————————————————————————————————————————————————————
