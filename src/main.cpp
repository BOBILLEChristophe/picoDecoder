/*

   picoDecoder

*/
#include <Arduino.h>
#include <ACAN2515.h>
// #include <SPI.h>

#define PROJECT "picoDecoder"
#define VERSION "0.3.0"
#define AUTHOR "Christophe BOBILLE - www.locoduino.org"

//----------------------------------------------------------------------------------------
//  Board Check
//----------------------------------------------------------------------------------------

#ifndef ARDUINO_ARCH_RP2040
#error "Select a Raspberry Pi Pico board"
#endif

const uint16_t thisHash = 0x00; // Identifiant unique du module (UID)

static const byte MCP2515_INT = 1;   // INT output of MCP2515
static const byte MCP2515_SCK = 18;  // SCK input of MCP2515
static const byte MCP2515_MOSI = 19; // SI input of MCP2515
static const byte MCP2515_MISO = 16; // SO output of MCP2515
static const byte MCP2515_CS = 17;   // CS input of MCP2515

// ——————————————————————————————————————————————————————————————————————————————
//   MCP2515 Driver object
// ——————————————————————————————————————————————————————————————————————————————

ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

// ——————————————————————————————————————————————————————————————————————————————
//   MCP2515 Quartz: adapt to your design
// ——————————————————————————————————————————————————————————————————————————————

static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 8 MHz

const uint8_t nbSensors = 16; // Choisir le nombre d'entrées souhaitées
const byte sensorInPin[nbSensors] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 20, 21};

// ——————————————————————————————————————————————————————————————————————————————
//    SETUP
// ——————————————————————————————————————————————————————————————————————————————

void setup()
{
  //--- Start serial
  Serial.begin(115200);
  //--- Wait for serial (blink led at 10 Hz during waiting)
  while (!Serial)
  {
    delay(50);
  }

  Serial.printf("\nProject   :    %s", PROJECT);
  Serial.printf("\nVersion   :    %s", VERSION);
  Serial.printf("\nAuteur    :    %s", AUTHOR);
  Serial.printf("\nFichier   :    %s", __FILE__);
  Serial.printf("\nCompiled  :    %s", __DATE__);
  Serial.printf(" - %s\n\n", __TIME__);
  Serial.printf("-----------------------------------\n\n");

  Serial.println("Start setup");

  //--- Configure SPI
  SPI.setSCK(MCP2515_SCK);
  SPI.setTX(MCP2515_MOSI);
  SPI.setRX(MCP2515_MISO);
  SPI.setCS(MCP2515_CS);
  SPI.begin();
  delay(1000);
  //--- Configure ACAN2515
  Serial.println("Configure ACAN2515");
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 250UL * 1000UL); // CAN bit rate 250 kb/s
  const uint32_t errorCode = can.begin(settings, []
                                       { can.isr(); });
  if (errorCode != 0)
  {
    Serial.print("Configuration error 0x");
    Serial.println(errorCode, HEX);
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
  { // MAJ des capteurs
    state &= ~(1 << i);
    if (!digitalRead(sensorInPin[i]))
      state |= 1 << i;
  }
  frame.data16[0] = state;
  frame.id = thisHash;
  frame.ext = 0;
  frame.len = 2;
  const bool ok = can.tryToSend(frame);
  if (ok)
    Serial.printf("Sent: %d\n", frame.data16[0]);
  else
    Serial.println("Send failure\n");
  delay(100); // Toutes les 100ms
}

// ——————————————————————————————————————————————————————————————————————————————
