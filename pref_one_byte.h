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

// Persistent Storage that stores 1 Byte of data in EEPROM utilizing defined area to reduce wear by cycling new values over the defined EEPROM area.
//
// DBYTE (Data Byte) is written cyclicly over free area.
// It is written as two bytes - one as is (DBYTE) and second is Redundance Byte RBYTE=(~DBYTE) to later validate for EEPROM corruptions.
//
// It is possible to define that only part of EEPROM is used for this storage with help of parameterized constructor.
// NB: Make sure to erase EEPROM when config changes to avoid library failures

// 0 - No Safety net
// 1 - Optional checks
// 2 - Algorithmic checks (they are covered by Unit tests and general testing)
// 3 - Paranoic checks (for absolute realiability - not to mess with EEPROM)
static constexpr uint8_t PREF_ONE_BYTE_SAFETY_CHECK_LEVEL = 2;

class PrefOneByte final {

  public:

    static constexpr auto& EMPTY_VALUE = EEPROMHelper::EMPTY_VALUE;
  
    enum ConfigError {
      CONFIG_OK, CE_START_CHUNK_ERROR, CE_CHUNK_SIZE_ERROR, CE_END_CHUNK_ERROR, CE_ASSUMPTION_FAILS, CE_UNIMPLEMENTED, CE_UNKNOWN_ERROR
    };
  
    enum Error { OK,
                  E01_CONFIG_ERROR,
                  E02_CONFIG_ERROR_1, E03_CONFIG_ERROR_2, E04_CONFIG_ERROR_3,
                  E05_ALG_ERROR_1, E06_ALG_ERROR_2,
                  E07_ALG_ERROR_ADDRESS__BAD_CHUNKNO, E08_ALG_ERROR_ADDRESS__BAD_LOCATION_OFFSET, E09_ALG_ERROR_BAD_ADDRESS,
                  E10_READ_EEPROM_CORRUPTED_REDUNDANCY_CHECK_ERROR,
                  E11_WRITE_ALG_ERR_1,
                  E13_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_1, // FIXME
                  E14_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_2,
                  E15_ERASE_WHOLE_CORRUPTED_1,
                  E16_ERASE_WHOLE_CORRUPTED_2,
                  E17_ERASE_WHOLE_CORRUPTED_3,
                  Exx_ADDR_OUT_OF_RANGE
               };

    // Default constructor assumes whole EEPROM will be used as a storage
    PrefOneByte();
    
    // Constructor: sets the area to use for storage
    // Chunk size must be >= 8 and odd number (Assumption A01)
    // For best performance configure chunk size as close as possible to total number of chunks
    // NOTE! Never modify config without erasing destination area first!
    //       For this - use cleanUpEeprom()
    PrefOneByte(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk);
  
    // Read latest `active` Data Byte,
    // returns 0xFF if EEPROM is empty
    uint8_t load();
  
    // Check if EEPROM isEmpty
    bool isEmpty();
  
    // Save preferences
    bool save(const uint8_t dataByte);
  
    // Check if last operation succeeded
    bool isSuccess() {
      return _lastError == OK;
    }

    // Returns Config Error (when wrong Constructor parameters provided)
    ConfigError getConfigError() {
      return _configError;
    }
  
    // Get last error happened after load / save / erase
    Error getLastError() {
      return _lastError;
    }
  
    // Erase this preference storage area in EEPROM
    // Use after configuration changed and EEPROM is not empty
    bool eraseStorage();

  private:
    // Optional Safety Checks (Algorithm checks)
    static constexpr bool __SC_O = (PREF_ONE_BYTE_SAFETY_CHECK_LEVEL >= 1);
    // Safety Check Logic Assumptions
    static constexpr bool __SC_LA = (PREF_ONE_BYTE_SAFETY_CHECK_LEVEL >= 2);
    // Safety Check Address Before every read/erase/write operation
    // Make sure that we are not writing to area of EEPROM not configured for
    //   this Performance Storage
    static constexpr bool __SC_ADDR = (PREF_ONE_BYTE_SAFETY_CHECK_LEVEL >= 3);


    // If configError != CONFIG_OK - no method will execute, and will return error = E01_CONFIG_ERROR
    ConfigError _configError;
    uint8_t _chunkSize = 0, _startChunk = 0, _endChunk = 0;
    
    Error _lastError = OK;

    void _constructor(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk);

    Error _preOperationCheck();

    // Reads latest `active` Data Byte, returns 0xFF if no data present in EEPROM yet or error occurs.
    Error _load(bool &isEEPROMEmpty, uint8_t &value);
    // Save preferences
    Error _save(const uint8_t dataByte);

    bool _loaded = false;
    bool _isEmpty;
    uint8_t _cachedDataByte;
  
    // In case some faulty code still uses this address - the chances are it is above physical address and will not corrupt actual EEPROM
    // We use F0 not FF because when incremented by misstake by 1 or other small number - it will not overflow to 0 or some other valid address.
    // TODO: Usage of this constant assumes that EEPROM size is not bigger then 0xFFF0, when rounded to available sizes it is 32KB
    //       If more is required - we can extend size of it to uint32_t 
    //                             or implement separate flag (its better from performance and ram usage perspectives)
    static constexpr uint16_t INVALID_ADDRESS = 0xFFF0; 

    bool __validateAssumptionA00();
    uint16_t calcStorageEndAddr(uint8_t chunkSize, uint8_t endChunk);
    uint8_t __calcRedundancyByte(uint8_t dByte);
    // Make sure address is within Storage area (as per configuration)
    bool __isAddressValid(uint16_t addr);
    bool __isValidChunkNo(uint8_t chunkNo);
    bool __areValidDAndRBytes(uint8_t dByte, uint8_t rByte);

    Error _validateDataByteAddress(uint8_t chunkNo, uint8_t locationOffset);
    uint16_t __calcDataByteAddress(uint8_t chunkNo, uint8_t locationOffset);
    
    Error _findActiveLocation(uint8_t &activeChunkNo, uint8_t &activeLocationOffset, bool &isEEPROMEmpty);
    Error _readAndValidateLocation(uint8_t chunkNo, uint8_t locationOffset, uint8_t &value);

    // 1) We do not erase first location of active chunk because it is needed to detect that chunk was busy
    // 2) FIXME: Make sure to RenderFirstLocationInvalid (this is extra safety to prevent this location from valid use)
    Error _eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(uint8_t activeChunkNo);

    // 1) We do not erase first byte in first chunk because it is already erased by main logic
    Error _eraseEveryFirstLocationOfEveryChunkGoingBackwards();

    Error _readByte(uint16_t addr, uint8_t &value) {
      if (__SC_ADDR && !__isAddressValid(addr)) return Exx_ADDR_OUT_OF_RANGE;
      value = EEPROMHelper::readByte(addr);
      return OK;
    }

    Error _eraseByte(uint16_t addr) {
      if (__SC_ADDR && !__isAddressValid(addr)) return Exx_ADDR_OUT_OF_RANGE;
      EEPROMHelper::eraseByte(addr);
      return OK;
    }

    Error _eraseByteIfNotEmpty(uint16_t addr) {
      if (__SC_ADDR && !__isAddressValid(addr)) return Exx_ADDR_OUT_OF_RANGE;
      EEPROMHelper::eraseByteIfNotEmpty(addr);
      return OK;
    }

    Error _bitwiseAndByte(uint16_t addr, uint8_t value) {
      if (__SC_ADDR && !__isAddressValid(addr)) return Exx_ADDR_OUT_OF_RANGE;
      EEPROMHelper::bitwiseAndByte(addr, value);
      return OK;
    }
};

#endif
