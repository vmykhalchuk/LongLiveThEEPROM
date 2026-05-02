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
#include "pref_one_byte.h"

// Implementation details:
// 
// EEPROM is split by chunks of CHUNK_SIZE each.
// Each chunk is split by two-byte locations. First byte is Data Byte second byte is Redundancy Byte.
// 
// In order to find location of active DByte - we split whole EEPROM area on smaller chunks CHUNK_SIZE bytes long.
// Then we check every first byte of each chunk starting from first chunk.
// The latest chunk where its first byte is occupied is scanned byte by byte to find latest active Data Byte.
// 
// When writing next location we check if current active chunk is full:
//   - If so - we erase all locations except first.
//   - We also erase redundancy byte of the first location in active chunk!
//     - Note: but we do not erase redundancy byte of first location of first chunk! (see writing logic)
//
// When all memory is exhausted - we erase every first byte of every chunk and all locations in last chunk.
// For detail explanation see comments in `writeDataByte` code.
//
//
// Assumptions:
//  A00: for DataByte == 0x00 -> Redundancy Byte is 0xFF
//  A00: for DataByte == 0xFF -> Redundancy Byte is 0x00
//  A01: Chunk size is even and >= 8
//
// TODO PRO Performance Improvement:
// In current implementation, when whole memory is utilized and next D-Byte is to be written
//   - we will have to erase all bytes in last chunk (CHUNK_SIZE erases) +
//     every first byte of each chunk (for 1K memory and chunk size = 32) it will be same CHUNK_SIZE erases
// To optimize it we can do extra steps when operating with last chunk:
//  - When searching for activeLocationOffset in last chunk, we check last location in last chunk first, and if not empty - we use it as active
//  - When writing to last chunk - as soon we have to write to last location (when one before last is active) - then we erase this whole chunk
//      but we leave first byte and of course last two bytes intact
//  - Next time we have to write after last location in last chunk - we do not have to clear whole active chunk - just beginning and end


PrefOneByte::PrefOneByte(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk) {
  _configError = UNKNOWN_ERROR; // when _configError != CONFIG_OK - all methods will return INIT_ERROR
  this->_chunkSize = 0; // when chunkSize == 0 - Initialization failed!

  uint16_t endChunkAddress = endChunk * chunkSize + (chunkSize - 1);
  
  if (endChunk > startChunk) {
    _configError = START_CHUNK_ERROR;
  } else if (chunkSize % 8 != 0) {
    _configError = CHUNK_SIZE_ERROR;
  } else if (endChunkAddress > E2END) {
    _configError = END_CHUNK_ERROR;
  } else {

    // no errors
    _configError = CONFIG_OK;
    this->_startChunk = startChunk;
    this->_endChunk = endChunk;
    this->_chunkSize = chunkSize;
  }

}

void PrefOneByte::_validateAssumptionA00(Error &error) {
  // [SAFETY CHECK/OPTIONAL] Read/Write code assumes that redundancy byte is following:
  if (_calcRedundancyByte(0x00) != 0xFF || _calcRedundancyByte(0xFF) != 0x00) {
    error = E12_WRITE_ALG_ERR_2;
    return;
  }
}

uint8_t PrefOneByte::_calcRedundancyByte(uint8_t dByte) {
  return ~dByte;
}

bool PrefOneByte::_isValidChunkNo(uint8_t chunkNo) {
  if (chunkNo > _endChunk) return false;
  if (chunkNo < _startChunk) return false;
  return true;
}

bool PrefOneByte::_areValidDAndRBytes(uint8_t dByte, uint8_t rByte) {
  return (rByte == _calcRedundancyByte(dByte));
}

// FIXME replace posNo with locationOffset in whole code (posNo is confusing) and location concept is described in docs
uint16_t PrefOneByte::_calcAndValidateDataByteAddress(uint8_t chunkNo, uint8_t locationOffset, Error &error) {

  { // [SAFETY CHECK] VALIDATE input data
    if (!_isValidChunkNo(chunkNo)) {
      error = E07_ALG_ERROR_ADDRESS__BAD_CHUNKNO;
    }
    if (locationOffset > (_chunkSize - 2)) {
      error = E08_ALG_ERROR_ADDRESS__BAD_POSNO;
    }
    if (error != OK) return INVALID_ADDRESS;
  }
  
  uint16_t address = ((uint16_t)chunkNo * _chunkSize) | locationOffset;

  { // [SAFETY CHECK]
    // This is DataByte, not RedundancyByte, thats why we decrease lastAllowedAddress by 1 to accomodate space for Redundancy Byte
    uint16_t lastAllowedAddress = (E2END - 1);
    if (address > lastAllowedAddress) {
      error = E09_ALG_ERROR_BAD_ADDRESS;
      return INVALID_ADDRESS;
    }
  }
  
  return address;
}

void PrefOneByte::_findActiveLocation(uint8_t &activeChunkNo, uint8_t &activeLocationOffset,
                                            bool &isEEPROMEmpty, Error &error) {

  // - FirstFree chunk/position is a first one where no data is written yet (0xFF)
  // - Active chunk/position is one with latest version of Data byte that was written
  //     (NB! Or beginning of EEPROM in case nothing was written yet)
  
  { // [SAFETY CHECK] Validate configs
    if (_chunkSize % 8 != 0) {
      error = E02_CONFIG_ERROR_1;
    } else if (_startChunk > _endChunk) {
      error = E03_CONFIG_ERROR_2;
    } else if (((_endChunk * _chunkSize) + _chunkSize - 1) > E2END) {
      error = E04_CONFIG_ERROR_3;
    }
    if (error != OK) return;
  }

  // Find first empty ChunkNo (NB: when no empty chunks left - (EEPROM_LAST_CHUNK + 1) is returned)
  uint8_t firstEmptyChunkNo = _startChunk;
  while (firstEmptyChunkNo <= _endChunk) {
    uint16_t locationAddr = (uint16_t) firstEmptyChunkNo * _chunkSize;
    uint8_t dataByte = _readByte(locationAddr);
    uint8_t redundancyByte = _readByte(locationAddr + 1);
    if (dataByte == EMPTY_BYTE && redundancyByte == EMPTY_BYTE) {
      // first location in chunk is empty => chunk is empty too
      break;
    }
    firstEmptyChunkNo++;
  }
  
  // Find Active Chunk
  activeChunkNo = firstEmptyChunkNo == _startChunk ? _startChunk : firstEmptyChunkNo - 1;

  // Calculate if EEPROM is Empty
  isEEPROMEmpty = firstEmptyChunkNo == _startChunk; // if Firt chunk is Empty - then whole EEPROM is Empty

  { // [SAFETY CHECK]
    if (!_isValidChunkNo(activeChunkNo)) {
      error = E05_ALG_ERROR_1;
      return;
    }
  }

  // Find First Free Position
  if (isEEPROMEmpty) {
    activeLocationOffset = 0;
  } else {
    uint8_t firstFreeLocationOffset = 0;
    uint16_t chunkStart = (uint16_t) activeChunkNo * _chunkSize;
    while (firstFreeLocationOffset < _chunkSize) {
      uint8_t dataByte = _readByte(chunkStart + firstFreeLocationOffset);
      uint8_t redundancyByte = _readByte(chunkStart + firstFreeLocationOffset + 1);
      if (dataByte == EMPTY_BYTE && redundancyByte == EMPTY_BYTE) {
        // location is empty
        break;
      }
      firstFreeLocationOffset += 2;
    }
    
    // Find Active Position (or set it to beginning if no Active yet)
    activeLocationOffset = firstFreeLocationOffset == 0 ? 0 : firstFreeLocationOffset - 2;
  }

  { // [SAFETY CHECK] Validate activePosNo
    if (activeLocationOffset > (_chunkSize - 2)) {
      error = E06_ALG_ERROR_2;
      return;
    }
  }

  //uint16_t activeAddress = chunkStart + activePosNo; // For safety we do not optimize but recalculate address

  if (!isEEPROMEmpty) {
    // [SAFETY CHECK] Recalculate & Validate activeAddress
    _calcAndValidateDataByteAddress(activeChunkNo, activeLocationOffset, error);
  }
}

uint8_t PrefOneByte::_readAndValidateLocation(uint8_t chunkNo, uint8_t locationOffset, Error &error) {
  
  uint16_t dAddr = _calcAndValidateDataByteAddress(chunkNo, locationOffset, error);
  if (error != OK) return FAULT_DATA_VALUE;

  { // [SAFETY CHECK / MANDATORY]
    uint8_t dByte = _readByte(dAddr);
    uint8_t rByte = _readByte(dAddr + 1);
    if (!_areValidDAndRBytes(dByte, rByte)) {
      error = E10_READ_EEPROM_CORRUPTED_REDUNDANCY_CHECK_ERROR;
    }
    
    return error == OK ? dByte : FAULT_DATA_VALUE;
  }
}

uint8_t PrefOneByte::_load(bool &isEEPROMEmpty, Error &error) {
  if (_configError != CONFIG_OK) {
    error = E01_INIT_ERROR;
    return FAULT_DATA_VALUE;
  }
  error = OK;
  
  // [SAFETY CHECK/OPTIONAL]
  _validateAssumptionA00(error);
  if (error != OK) return FAULT_DATA_VALUE;

  uint8_t activeChunkNo;
  uint8_t activeLocationOffset;
  _findActiveLocation(activeChunkNo, activeLocationOffset, isEEPROMEmpty, error);
  if (error != OK) return FAULT_DATA_VALUE;
  
  if (isEEPROMEmpty) {
    return FAULT_DATA_VALUE;
  } else {
    return _readAndValidateLocation(activeChunkNo, activeLocationOffset, error);
  }
}

void PrefOneByte::_eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(uint8_t activeChunkNo, Error &error) {

  uint8_t locOffset = _chunkSize - 2;
  while (true) {
    
    uint16_t dAddr = _calcAndValidateDataByteAddress(activeChunkNo, locOffset, error);
    if (error != OK) return;
    uint16_t rAddr = dAddr + 1;

    uint8_t dByte = _readByte(dAddr);
    uint8_t rByte = _readByte(rAddr);

    // [SAFETY CHECK]
    if (!_areValidDAndRBytes(dByte, rByte)) {
      error = E13_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_1;
      return;
    }

    if (locOffset == 0) {
      // case #1
      // First location - we must keep at least one non-empty (D-Byte or R-Byte) to keep marker that this Chunk is not Empty
      // However it also should be invalid - in case something goes wrong - so it will not be picked up as active value
      if (dByte == EMPTY_BYTE || rByte == EMPTY_BYTE) {
        // no need to erase anything (one of is already empty)
        // however we need to invalidate location
        if (dByte == EMPTY_BYTE) {
          _bitwiseAndByte(dAddr, 0xEE);
          _eraseByte(rAddr);
        } else {
          _bitwiseAndByte(rAddr, 0xEE);
          _eraseByte(dAddr);
        }
      } else {
        // erase R-Byte
        _eraseByte(rAddr);
      }

      // [SAFETY CHECK / OPTIONAL] Double check : Validate that location is indeed invalid
      {
        uint8_t dByte = _readByte(dAddr);
        uint8_t rByte = _readByte(rAddr);
        if (_areValidDAndRBytes(dByte, rByte)) {
          error = E14_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_2;
          return;
        }
      }
      
    } else {

      // erase D-Byte
      if (dByte != EMPTY_BYTE) _eraseByte(dAddr);
      // erase R-Byte
      if (rByte != EMPTY_BYTE) _eraseByte(rAddr);
    }

    if (locOffset == 0) {
      break;
    } else {
      locOffset -= 2;
    }
  }
}

void PrefOneByte::_eraseEveryFirstLocationOfEveryChunkGoingBackwards(Error &error) {
  
  uint8_t chunkNo = _endChunk;
  while (true) {
    
    uint16_t dAddr = _calcAndValidateDataByteAddress(chunkNo, 0, error);
    if (error != OK) return;
    uint16_t rAddr = dAddr + 1;

    uint8_t dByte = _readByte(dAddr);
    uint8_t rByte = _readByte(rAddr);

    // [SAFETY CHECK] We expect location to be invalid (this is old location, not active)
    if (_areValidDAndRBytes(dByte, rByte)) {
      error = E15_ERASE_WHOLE_CORRUPTED_1;
      return;
    }

    if (dByte == EMPTY_BYTE && rByte == EMPTY_BYTE) {
      // something strange here, should be invalid location, not empty
      error = E16_ERASE_WHOLE_CORRUPTED_2;
      return;
      
    } else if (dByte != EMPTY_BYTE && rByte != EMPTY_BYTE) {
      // looks like some logic failed - we need to erase both bytes which is unexpected!
      error = E17_ERASE_WHOLE_CORRUPTED_3;
      return;
      
    } else {
      
      // erase D-Byte
      if (dByte != EMPTY_BYTE) _eraseByte(dAddr);
      // erase R-Byte
      if (rByte != EMPTY_BYTE) _eraseByte(rAddr);
    }

    if (chunkNo == _startChunk) {
      break;
    } else {
      chunkNo--;
    }
  }

}

bool PrefOneByte::_save(const uint8_t newDataByte, Error &error) {
  if (_configError != CONFIG_OK) {
    error = E01_INIT_ERROR;
    return false;
  }
  error = OK;
  
  // [SAFETY CHECK/OPTIONAL]
  _validateAssumptionA00(error);
  if (error != OK) return false;

  bool isEEPROMEmpty;
  uint8_t activeChunkNo;
  uint8_t activeLocationOffset;
  _findActiveLocation(activeChunkNo, activeLocationOffset, isEEPROMEmpty, error);
  if (error != OK) return false;

  uint8_t activeDataByte = newDataByte;
  { // [SAFETY CHECK] // Validate data in active location
    if (!isEEPROMEmpty)
        activeDataByte = _readAndValidateLocation(activeChunkNo, activeLocationOffset, error);
    if (error != OK) return false;
  }

  // Check if stored value is not equal to new value
  if (!isEEPROMEmpty && (activeDataByte == newDataByte)) {
    return true;
  }

  // Calculate if Active chunk is full - mark that Active chunk has to be erased
  bool eraseActiveChunkNeeded = false;
  uint8_t targetLocationOffset;
  if (isEEPROMEmpty) {
    targetLocationOffset = 0;
  } else if (activeLocationOffset == _chunkSize - 2) {
    // end of Chunk reached
    eraseActiveChunkNeeded = true;
    targetLocationOffset = 0;
  } else {
    targetLocationOffset = activeLocationOffset + 2;
  }

  // Calculate if EEPROM is Full - mark that Whole EEPROM has to be erased
  bool eraseWholeNeeded = false;
  uint8_t targetChunkNo = activeChunkNo;
  if (eraseActiveChunkNeeded) {
    if (activeChunkNo == _endChunk) {
      // end of EEPROM reached (defined in config)
      eraseWholeNeeded = true;
      targetChunkNo = _startChunk;
    } else {
      targetChunkNo = activeChunkNo + 1;
    }
  }

  const uint16_t targetAddress = _calcAndValidateDataByteAddress(targetChunkNo, targetLocationOffset, error);
  if (error != OK) return false;

    // Four possible scenario:
    // 0) EEPROM is empty
    //    20. If dataByte=0x00 THEN Write target D-Byte ELSE Write target R-Byte
    //    21. If dataByte=0x00 THEN Write target R-Byte ELSE Write target D-Byte
    // 1) Writing to beginning because EEPROM is full (eraseWholeNeeded = true)
    //    10. Erase Active Chunk (going backward)
    //    11. Erase Every First Location (going backwards)
    //    20. If dataByte=0x00 THEN Write target D-Byte ELSE Write target R-Byte
    //    21. If dataByte=0x00 THEN Write target R-Byte ELSE Write target D-Byte
    // 2) When required to advance to next chunk (active chunk is full)
    //    10. Erase Active Chunk (going backward)
    //    20. If dataByte=0x00 THEN Write target D-Byte ELSE Write target R-Byte
    //    21. If dataByte=0x00 THEN Write target R-Byte ELSE Write target D-Byte
    // 3) otherwise
    //    20. If dataByte=0x00 THEN Write target D-Byte ELSE Write target R-Byte
    //    21. If dataByte=0x00 THEN Write target R-Byte ELSE Write target D-Byte
    
  // step 10
  if (eraseActiveChunkNeeded) {
    _eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(activeChunkNo, error);
    if (error != OK) return false;
  }

  // step 11
  if (eraseWholeNeeded) {
    _eraseEveryFirstLocationOfEveryChunkGoingBackwards(error);
    if (error != OK) return false;
  }
  
  // step 2 (20 & 21)
  {
    { // [SAFETY CHECK/MANDATORY] Target location at this point must be empty!
      const uint8_t targetDataByte = _readByte(targetAddress);
      const uint8_t targetRedundancyByte = _readByte(targetAddress + 1);
      if (targetDataByte != EMPTY_BYTE || targetRedundancyByte != EMPTY_BYTE) {
        error = E11_WRITE_ALG_ERR_1; // FIXME This is either EEPROM Corruption or ALG error
        return false;
      }
    }
    
    if (newDataByte == 0x00 || newDataByte == 0xFF) {
      _bitwiseAndByte(newDataByte == 0x00 ? targetAddress : targetAddress + 1, 0x00);
    } else {
      _bitwiseAndByte(targetAddress, newDataByte);
      _bitwiseAndByte(targetAddress + 1, _calcRedundancyByte(newDataByte));
    }
  }
  
  // SUCCESS
  return true;
}

bool PrefOneByte::eraseStorage() {
  if (_configError != CONFIG_OK) return false;
  
  for (uint8_t chunkNo = _startChunk; chunkNo <= _endChunk; chunkNo++) {
    uint16_t chunkAddress = chunkNo * _chunkSize;
    for (uint8_t j = 0; j < _chunkSize; j++) {
      EEPROMHelper::eraseByteIfNotEmpty(chunkAddress + j);
    }
  }
  return true;
}

uint8_t PrefOneByte::load() {
  if (!_initialized) {
    _cachedDataByte = _load(_isEmpty, _lastError);
    _initialized = true;
  }
  return _cachedDataByte;
}

bool PrefOneByte::isEmpty() {
  if (!_initialized) {
    load();
  }
  return _isEmpty;
}

bool PrefOneByte::save(const uint8_t dataByte) {
  _initialized = false;
  return _save(dataByte, _lastError);
}
