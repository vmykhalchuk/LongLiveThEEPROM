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
#ifndef PREF_ONE_BYTE_H
#define PREF_ONE_BYTE_H

#include <Arduino.h>
#include "eeprom_helper.h"

// Functions that store 1 Byte of data in EEPROM utilizing defined area to reduce wear by cycling new values over the area.
//
// DBYTE (Data Byte) is written cyclicly over free area.
// It is written as two bytes - one as is (DBYTE) and second is Redundance Byte RBYTE=(~DBYTE) to later validate for EEPROM corruptions.
//
// It is possible to define that only part of EEPROM is used for this storage by modifying EEPROM_STARTING_CHUNK and EEPROM_LAST_CHUNK (and CHUNK_SIZE_MAGNITUDE).
// NB: Make sure to erase EEPROM when config changes to avoid library failures

// Library currently supports up to 32KB EEPROM (see INVALID_ADDRESS constant for details on why)

class PrefOneByte {
  
  public:

  static constexpr auto& EMPTY_BYTE = EEPROMHelper::EMPTY_BYTE;
  
  // Constructor: sets the area to use for storage
  //PrefOneByte(uint8_t chunkSizeMag, uint8_t startChunk, uint8_t endChunk);

  // Chunk size must be >= 8 and odd number (Assumption A01)
  // For best performance configure chunk size as close as possible to total number of chunks
  // NOTE! Never modify config without erasing destination area first!
  //       For this - use cleanUpEeprom()
  //PrefOneByte(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk);

  /*
  enum ConfigError {
    CONFIG_OK, END_CHUNK_ERROR, CHUNK_SIZE_ERROR, UNKNOWN_ERROR
  };*/

  // For 1024 size 32 is recommended
  // For 4096 size 64 is recommended
  // For 16384 size 128 is recommended
  static const uint8_t CHUNK_SIZE_MAGNITUDE = 5; // {min 3, max 8} 5 for 32, 6 for 64, 7 for 128
  
  static const uint8_t EEPROM_STARTING_CHUNK = 0;
  static const uint8_t EEPROM_LAST_CHUNK = EEPROMHelper::EEPROM_SIZE / (1 << CHUNK_SIZE_MAGNITUDE) - 1;


  enum Error { OK,
                E01_CONFIG_ERROR, E02_CONFIG_ERROR, E03_CONFIG_ERROR,
                E04_ALG_ERROR, E05_ALG_ERROR,
                E06_ALG_ERROR_ADDRESS__BAD_CHUNKNO, E07_ALG_ERROR_ADDRESS__BAD_POSNO, E08_ALG_ERROR_BAD_ADDRESS,
                E09_READ_EEPROM_CORRUPTED_REDUNDANCY_CHECK_ERROR,
                E10_WRITE_ALG_ERR_1,
                E11_WRITE_ALG_ERR_2,
                E12_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_1,
                E13_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_2,
                E14_ERASE_WHOLE_CORRUPTED_1,
                E15_ERASE_WHOLE_CORRUPTED_2,
                E16_ERASE_WHOLE_CORRUPTED_3
             };

  // Reads latest `active` Data Byte, returns 0xFF if no data present in EEPROM yet or error occurs.
  uint8_t load(bool &isEEPROMEmpty, Error &error);

  // Read latest `active` Data Byte, returns 0xFF
  uint8_t load();

  // Validate if EEPROM isEmpty
  bool isEmpty();

  // Save preferences
  bool save(const uint8_t dataByte, Error &error);

  // Save preferences
  bool save(const uint8_t dataByte);

  // Check if last operation succeeded
  bool isSuccess() {
    return _lastError == OK;
  }

  // Get last error happened after load / save
  Error getLastError() {
    return _lastError;
  }

  // Clean EEPROM area designated for this data storage
  void cleanUpEeprom();

  static const uint8_t CHUNK_SIZE = 1 << CHUNK_SIZE_MAGNITUDE;

  private:

    Error _lastError = OK;

    bool _initialized = false;
    bool _isEmpty;
    uint8_t _cachedDataByte;
  
    //uint8_t chunkSize = 0, uint8_t startChunk = 0, uint8_t endChunk = 0;
  
    // In case some faulty code still uses this address - the chances are it is above physical address and will not corrupt actual EEPROM
    // We use F0 not FF because when incremented by misstake by 1 or other small number - it will not overflow to 0 or some other valid address.
    // TODO: Usage of this constant assumes that EEPROM size is not bigger then 0xFFF0, when rounded to available sizes it is 32KB
    //       If more is required - we can extend size of it to uint32_t 
    //                             or implement separate flag (its better from performance and ram usage perspectives)
    static const uint16_t INVALID_ADDRESS = 0xFFF0; 

    static const uint8_t FAULT_DATA_VALUE = 0xFF;

    static void _initError(Error &err);
    static uint8_t _calcRedundancyByte(uint8_t dByte);
    static void _validateAssumptionA00(Error &error);
    static bool _isValidChunkNo(uint8_t chunkNo);
    static bool _areValidDAndRBytes(uint8_t dByte, uint8_t rByte);

    // FIXME replace posNo with locationOffset in whole code (posNo is confusing) and location concept is described in docs
    static uint16_t _calcAndValidateDataByteAddress(uint8_t chunkNo, uint8_t locationOffset, Error &error);
    static void _findActiveLocation(uint8_t &activeChunkNo, uint8_t &activeLocationOffset, bool &isEEPROMEmpty, Error &error);
    static uint8_t _readAndValidateLocation(uint8_t chunkNo, uint8_t locationOffset, Error &error);

    // 1) We do not erase first location of active chunk because it is needed to detect that chunk was busy
    // 2) FIXME: Make sure to RenderFirstLocationInvalid (this is extra safety to prevent this location from valid use)
    static void _eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(uint8_t activeChunkNo, Error &error);

    // 1) We do not erase first byte in first chunk because it is already erased by main logic
    static void _eraseEveryFirstLocationOfEveryChunkGoingBackwards(Error &error);

    static uint8_t _readByte(uint16_t addr) {
      return EEPROMHelper::readByte(addr);
    }

    static void _eraseByte(uint16_t addr) {
      EEPROMHelper::eraseByte(addr);
    }

    static void _bitwiseAndByte(uint16_t addr, uint8_t value) {
      EEPROMHelper::bitwiseAndByte(addr, value);
    }
};

#endif
