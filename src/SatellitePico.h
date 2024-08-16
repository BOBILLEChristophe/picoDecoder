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

#ifndef __SATELLITEPICO_H__
#define __SATELLITEPICO_H__

#include <ACAN2515.h>
#include <Arduino.h>

static const uint32_t k2515ClockFrequency = 16'000'000UL;

class SatellitePico {
private:
  const uint8_t mcp2515_int = 9;
  const uint8_t mcp2515_sck = 10;
  const uint8_t mcp2515_mosi = 11;
  const uint8_t mcp2515_miso = 12;
  const uint8_t mcp2515_cs = 13;

public:
  ACAN2515 can;
  SatellitePico() : can(mcp2515_cs, SPI1, mcp2515_int) {}
  uint16_t begin(const ACAN2515Settings &inSettings);
  void selfTest();
};

void printACAN2515Error(Stream &destStream, uint16_t error);

extern SatellitePico gSat;

#endif /* __SATELLITEPICO_H__ */
