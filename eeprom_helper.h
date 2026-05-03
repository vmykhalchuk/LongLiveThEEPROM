/*
 * Copyright 2026 Volodymyr Mykhalchuk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EEPROM_HELPER_H
#define EEPROM_HELPER_H

#include <avr/interrupt.h>
#include <avr/io.h>

class EEPROMHelper final {

  public:

    static constexpr uint16_t EEPROM_SIZE = E2END + 1;
    static constexpr uint8_t EMPTY_VALUE = 0xFF;

    static bool isBusy();

    static void eraseByte(uint16_t addr);

    static void eraseByteIfNotEmpty(uint16_t addr);

    // Clear bits in byte by address.
    // Effectively performs AND bitwise operation against byte in EEPROM.
    //   0 bits → will be written (clear)
    //   1 bits → unchanged
    static void bitwiseAndByte(uint16_t addr, uint8_t mask);
  
    static uint8_t readByte(uint16_t addr);

  private:

};

#endif
