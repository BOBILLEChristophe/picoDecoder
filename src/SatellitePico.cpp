/*
 * Satellite Pico Library
 *
 * Copyright Jean-Luc Béchennec 2024
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Please read the LICENCE-GPLv2 or LICENCE-GPLv3 file.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef ARDUINO_ARCH_RP2040
#error "Selectionnez une carte avec un MCU RP2040"
#endif

#if F_CPU != 128000000L
#error "CPU frequency is wrong. It should be set to 128 MHz"
#endif

#include "SatellitePico.h"
#include <hardware/pwm.h>

static const uint32_t kClockPin2515 = 22;

/*------------------------------------------------------------------------------
 * begin has to be called to start the CAN.
 * begin does the following:
 * - program GPIO 22 to output a 16MHz clock using a PWM. GPIO 22 is
 *   connected to the clock input of the MCP2515.
 * - setup SPI1 on pins used by the MCP2515.
 * - start SPI1.
 * - start the MCP2515 with the settings given as argument.
 * - return the errorCode of the CAN network begin function.
 */
uint16_t SatellitePico::begin(const ACAN2515Settings &inSettings) {
  /* Maximum output current for the clock signal of MCP2515 */
  gpio_set_drive_strength(kClockPin2515, GPIO_DRIVE_STRENGTH_12MA);
  /* It is a PWM function */
  gpio_set_function(kClockPin2515, GPIO_FUNC_PWM);
  /* Get Slice and Channel of the pin */
  const int sliceClockPin2515 = pwm_gpio_to_slice_num(kClockPin2515);
  const int channelClockPin2515 = pwm_gpio_to_channel(kClockPin2515);
  /* 128MHz clock is divided by 4 -> 32 MHz */
  pwm_set_clkdiv_int_frac(sliceClockPin2515, 4, 0);
  /* Value at which the counter is reset to 0 (period = value + 1)
   * period = 32 / (1/1) = 16 MHz */
  pwm_set_wrap(sliceClockPin2515, 1);
  /* duty cycle = 1 / (1 + 1) = 50% */
  pwm_set_chan_level(sliceClockPin2515, channelClockPin2515, 1);
  /* Start the PWM */
  pwm_set_enabled(sliceClockPin2515, true);
  /* Wait for the MCP2515 startup (see § 8.1 of MCP2515 datasheet) */
  delay(1);

  SPI1.setSCK(mcp2515_sck);
  SPI1.setTX(mcp2515_mosi);
  SPI1.setRX(mcp2515_miso);
  SPI1.setCS(mcp2515_cs);

  SPI1.begin();

  const uint16_t errorCode = can.begin(inSettings, [] { gSat.can.isr(); });

  return errorCode;
}

/*------------------------------------------------------------------------------
 * selfTest performs a test of the SatellitePico module.
 * selfTest does the following:
 * - flash the Pico builtin LED 10 times à 20kHz.
 * - call begin with a loopback configuration.
 * - send and receive a set of 20 CAN messages, 1 every 100ms,
 *   and flash the Pico built-in led at 10kHz.
 * - call end.
 * - flash the Pico builtin LED à 2kHz.
 */
void SatellitePico::selfTest() {

  /* Notify the self test begin */
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint32_t i = 0; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(25);
    digitalWrite(LED_BUILTIN, LOW);
    delay(25);
  }

  /* begin the CAN */
  ACAN2515Settings settings(k2515ClockFrequency, 250UL * 1000UL);
  //settings.mRequestedMode = ACAN2515Settings::LoopBackMode;
  const uint16_t errorCode = gSat.begin(settings);

  /* Send 20 messages and receive them */
  for (uint32_t i = 0; i < 20; i++) {
    uint32_t date = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    CANMessage frame;
    frame.data[0] = i;
    frame.len = 1;
    const bool ok = gSat.can.tryToSend(frame);
    if (ok) {
      while (!gSat.can.available())
        ;
      gSat.can.receive(frame);
      if (!(frame.len == 1 && frame.data[0] == i)) {
        /* If received frame is not the sent one, stop */
        while (1)
          ;
      }
      digitalWrite(LED_BUILTIN, LOW);
      while (millis() - date < 100)
        ;
    }
  }

  /* Signal the end of the test */
  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}

/*------------------------------------------------------------------------------
 * printACAN2515Error print the error meaning on a Stream
 */
void printACAN2515Error(Stream &destStream, uint16_t error) {
  if (error & ACAN2515::kNoMCP2515)
    destStream.println("MCP2515 is missing");
  if (error & ACAN2515::kTooFarFromDesiredBitRate)
    destStream.println("Configuration too far from desired bitrate");
  if (error & ACAN2515::kInconsistentBitRateSettings)
    destStream.println("Inconsistent bitrate settings");
  if (error & ACAN2515::kINTPinIsNotAnInterrupt)
    destStream.println("Interrupt pin cannot handle interrupts");
  if (error & ACAN2515::kISRIsNull)
    destStream.println("ISR function is NULL");
  if (error & ACAN2515::kRequestedModeTimeOut)
    destStream.println("Switching to requested mode made a timeout");
  if (error & ACAN2515::kAcceptanceFilterArrayIsNULL)
    destStream.println("Acceptance filter array is NULL");
  if (error & ACAN2515::kOneFilterMaskRequiresOneOrTwoAcceptanceFilters)
    destStream.println("One filter mask requires 1 or 2 acceptance filter(s)");
  if (error & ACAN2515::kTwoFilterMasksRequireThreeToSixAcceptanceFilters)
    destStream.println("Two filters mask require 3 to 6 acceptance filters");
  if (error & ACAN2515::kCannotAllocateReceiveBuffer)
    destStream.println("Not enough RAM to allocate the receive buffer");
  if (error & ACAN2515::kCannotAllocateTransmitBuffer0)
    destStream.println("Not enough RAM to allocate the transmit buffer 0");
  if (error & ACAN2515::kCannotAllocateTransmitBuffer1)
    destStream.println("Not enough RAM to allocate the transmit buffer 1");
  if (error & ACAN2515::kCannotAllocateTransmitBuffer2)
    destStream.println("Not enough RAM to allocate the transmit buffer 2");
  if (error & ACAN2515::kISRNotNullAndNoIntPin)
    destStream.println(
        "ISR is not NULL and there is no interrupt pin provided");
}

SatellitePico gSat;
