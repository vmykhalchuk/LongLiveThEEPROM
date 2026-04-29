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

#include "eeprom_helper.h"

bool EEPROMHelper::isBusy() {
  return (EECR & (1 << EEPE));
}

void EEPROMHelper::eraseByte(uint16_t addr) {
  // Wait for completion of previous erase/write
  while (isBusy());

  // Set address
  EEAR = addr;

  EEDR = EMPTY_BYTE; // For simavr only, no effect on real hardware

  // Set erase-only mode (EEPM1:0 = 01)
  EECR = (EECR & ~((1 << EEPM1) | (1 << EEPM0))) | (1 << EEPM0);

  uint8_t sreg = SREG;
  cli();
    
  // Enable master write
  EECR |= (1 << EEMPE);
  // Start erase
  EECR |= (1 << EEPE);

  SREG = sreg;
}

void EEPROMHelper::eraseByteIfNotEmpty(uint16_t addr) {
  if (readByte(addr) != EMPTY_BYTE) {
    eraseByte(addr);
  }
}
  
void EEPROMHelper::bitwiseAndByte(uint16_t addr, uint8_t mask) {
  // Wait for completion of previous erase/write
  while (isBusy());

  // Load address
  EEAR = addr;

  // Load data:
  // 0 bits → will be written (clear)
  // 1 bits → unchanged
  EEDR = mask;

  // Set write-only mode (EEPM1:0 = 10)
  EECR = (EECR & ~((1 << EEPM1) | (1 << EEPM0))) | (1 << EEPM1);

  uint8_t sreg = SREG;
  cli();

  // Enable master write
  EECR |= (1 << EEMPE);
  // Start write
  EECR |= (1 << EEPE);

  SREG = sreg;
}

uint8_t EEPROMHelper::readByte(uint16_t addr) {
  // Wait for completion of previous erase/write
  while (isBusy());
    
  EEAR = addr;
  EECR |= (1 << EERE);
  return EEDR;
}
