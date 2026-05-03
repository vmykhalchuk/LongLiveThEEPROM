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
//  A02: Library currently supports up to 65,025=255*255 EEPROM size (chunkSize[0..255] * endChunk[0..255])
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

PrefOneByte::PrefOneByte() {
  uint8_t chunkSize = 0;
  switch (EEPROMHelper::EEPROM_SIZE) {
    case 256:   chunkSize = 16; break;  // chunkNo[0..15]
    case 512:   chunkSize = 32; break;  // chunkNo[0..15]
    case 1024:  chunkSize = 32; break;  // chunkNo[0..31]
    case 2048:  chunkSize = 64; break;  // chunkNo[0..31]
    case 4096:  chunkSize = 64; break;  // chunkNo[0..63]
    case 8192:  chunkSize = 128; break; // chunkNo[0..63]
    case 16384: chunkSize = 128; break; // chunkNo[0..127]
    case 32768: chunkSize = 128; break; // chunkNo[0..255]
    default:
      _configError = CE_UNIMPLEMENTED;
      return;
  }
  uint8_t startChunk = 0;
  uint8_t endChunk = (EEPROMHelper::EEPROM_SIZE / chunkSize) - 1;
  _constructor(chunkSize, startChunk, endChunk);
}

PrefOneByte::PrefOneByte(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk) {
  _constructor(chunkSize, startChunk, endChunk);
}

void PrefOneByte::_constructor(uint8_t chunkSize, uint8_t startChunk, uint8_t endChunk) {
  _configError = CE_UNKNOWN_ERROR; // when _configError != CONFIG_OK - all methods will return CONFIG_ERROR
  this->_chunkSize = 0; // when chunkSize == 0 - Initialization failed!

  if (__SC_LA) {
    if (!__validateAssumptionA00()) {
      // A00 fails
      _configError = CE_ASSUMPTION_FAILS;
      return;
    }
  }
  
  if (chunkSize < 8 || chunkSize % 2 != 0) { // A01
    _configError = CE_CHUNK_SIZE_ERROR;
  } else if (startChunk > endChunk) {
    _configError = CE_START_CHUNK_ERROR;
  } else if (calcStorageEndAddr(chunkSize, endChunk) > E2END) {
    _configError = CE_END_CHUNK_ERROR;
  } else {

    // no errors
    _configError = CONFIG_OK;
    this->_startChunk = startChunk;
    this->_endChunk = endChunk;
    this->_chunkSize = chunkSize;
  }
}

#define CHECK_RET(methodCall) { Error e = methodCall; if (Error::OK != e) return e; }

bool PrefOneByte::__validateAssumptionA00() {
  if (__calcRedundancyByte(0x00) != 0xFF || __calcRedundancyByte(0xFF) != 0x00) {
    return false;
  }
  return true;
}

uint16_t PrefOneByte::calcStorageEndAddr(uint8_t chunkSize, uint8_t endChunk) {
  return (uint16_t) endChunk * chunkSize + (chunkSize - 1);
}

uint8_t PrefOneByte::__calcRedundancyByte(uint8_t dByte) {
  return ~dByte;
}

bool PrefOneByte::__isAddressValid(uint16_t addr) {
  if (_configError != CONFIG_OK) return false;
  
  const uint16_t startAddr = _startChunk * _chunkSize;
  const uint16_t endAddr = calcStorageEndAddr(_chunkSize, _endChunk);
  
  return (startAddr < endAddr) && (endAddr < EEPROMHelper::EEPROM_SIZE)
          && (addr >= startAddr) && (addr <= endAddr);
}

bool PrefOneByte::__isValidChunkNo(uint8_t chunkNo) {
  if (chunkNo > _endChunk) return false;
  if (chunkNo < _startChunk) return false;
  return true;
}

bool PrefOneByte::__areValidDAndRBytes(uint8_t dByte, uint8_t rByte) {
  return (rByte == __calcRedundancyByte(dByte));
}

PrefOneByte::Error PrefOneByte::_validateDataByteAddress(uint8_t chunkNo, uint8_t locationOffset) {
  if (__SC_O) {
    if (!__isValidChunkNo(chunkNo)) {
      return E07_ALG_ERROR_ADDRESS__BAD_CHUNKNO;
    }
    if (locationOffset > (_chunkSize - 2)) {
      return E08_ALG_ERROR_ADDRESS__BAD_LOCATION_OFFSET;
    }
  }
  
  uint16_t resAddress = __calcDataByteAddress(chunkNo, locationOffset);

  if (__SC_O) {
    // resAddress is address for DataByte, we also should check for RedundancyByte address
    if (!__isAddressValid(resAddress) || !__isAddressValid(resAddress + 1)) {
      return E09_ALG_ERROR_BAD_ADDRESS;
    }
  }
  
  return OK;
}

uint16_t PrefOneByte::__calcDataByteAddress(uint8_t chunkNo, uint8_t locationOffset) {
  return ((uint16_t)chunkNo * _chunkSize) | locationOffset;
}

PrefOneByte::Error PrefOneByte::_findActiveLocation(uint8_t &activeChunkNo, uint8_t &activeLocationOffset,
                                            bool &isEEPROMEmpty) {

  // - FirstFree chunk/location is a first one where no data is written yet (0xFF)
  // - Active chunk/location is one with latest version of Data byte that was written
  //     (NB! Or beginning of EEPROM in case nothing was written yet)
  
  // Find first empty ChunkNo (NB: when no empty chunks left - (EEPROM_LAST_CHUNK + 1) is returned)
  uint8_t firstEmptyChunkNo = _startChunk;
  while (firstEmptyChunkNo <= _endChunk) {
    uint16_t locationAddr = (uint16_t) firstEmptyChunkNo * _chunkSize;
    uint8_t dataByte, redundancyByte;
    CHECK_RET(_readByte(locationAddr, dataByte));
    CHECK_RET(_readByte(locationAddr + 1, redundancyByte));
    
    if (dataByte == EMPTY_VALUE && redundancyByte == EMPTY_VALUE) {
      // first location in chunk is empty => chunk is empty too
      break;
    }
    firstEmptyChunkNo++;
  }
  
  // Find Active Chunk
  activeChunkNo = firstEmptyChunkNo == _startChunk ? _startChunk : firstEmptyChunkNo - 1;

  // Calculate if EEPROM is Empty
  isEEPROMEmpty = firstEmptyChunkNo == _startChunk; // if Firt chunk is Empty - then whole EEPROM is Empty

  if (__SC_O) {
    if (!__isValidChunkNo(activeChunkNo)) {
      return E05_ALG_ERROR_1;
    }
  }

  // Find First Free Location
  if (isEEPROMEmpty) {
    activeLocationOffset = 0;
  } else {
    uint8_t firstFreeLocationOffset = 0;
    uint16_t chunkStart = (uint16_t) activeChunkNo * _chunkSize;
    while (firstFreeLocationOffset < _chunkSize) {
      uint8_t dataByte, redundancyByte;
      CHECK_RET(_readByte(chunkStart + firstFreeLocationOffset, dataByte));
      CHECK_RET(_readByte(chunkStart + firstFreeLocationOffset + 1, redundancyByte));
      
      if (dataByte == EMPTY_VALUE && redundancyByte == EMPTY_VALUE) {
        // location is empty
        break;
      }
      firstFreeLocationOffset += 2;
    }
    
    // Find Active Position (or set it to beginning if no Active yet)
    activeLocationOffset = firstFreeLocationOffset == 0 ? 0 : firstFreeLocationOffset - 2;
  }

  if (__SC_O) {
    if (activeLocationOffset > (_chunkSize - 2)) {
      return E06_ALG_ERROR_2;
    }
  }

  //uint16_t activeAddress = chunkStart + activePosNo; // For safety we do not optimize but force address recalculation down the code

  if (!isEEPROMEmpty) {
    if (__SC_O) {
      CHECK_RET(_validateDataByteAddress(activeChunkNo, activeLocationOffset));
    }
  }

  return OK;
}

PrefOneByte::Error PrefOneByte::_readAndValidateLocation(uint8_t chunkNo, uint8_t locationOffset, uint8_t &value) {
  value = EMPTY_VALUE;
  
  if (__SC_O) {
    CHECK_RET(_validateDataByteAddress(chunkNo, locationOffset));
  }
  uint16_t dAddr = __calcDataByteAddress(chunkNo, locationOffset);

  uint8_t dByte, rByte;
  CHECK_RET(_readByte(dAddr, dByte));
  CHECK_RET(_readByte(dAddr + 1, rByte));

  if (!__areValidDAndRBytes(dByte, rByte)) {
    return E10_READ_EEPROM_CORRUPTED_REDUNDANCY_CHECK_ERROR;
  }

  value = dByte;
  return OK;
}

PrefOneByte::Error PrefOneByte::_preOperationCheck() {
  return _configError != CONFIG_OK ? E01_CONFIG_ERROR : OK;
}

PrefOneByte::Error PrefOneByte::_load(bool &isEEPROMEmpty, uint8_t &value) {
  value = EMPTY_VALUE;
  CHECK_RET(_preOperationCheck());
  
  uint8_t activeChunkNo;
  uint8_t activeLocationOffset;
  CHECK_RET(_findActiveLocation(activeChunkNo, activeLocationOffset, isEEPROMEmpty));
  
  if (isEEPROMEmpty) {
    return OK;
  } else {
    CHECK_RET(_readAndValidateLocation(activeChunkNo, activeLocationOffset, value));
    return OK;
  }
}

PrefOneByte::Error PrefOneByte::_eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(uint8_t activeChunkNo) {

  uint8_t locOffset = _chunkSize - 2;
  while (true) {

    if (__SC_O) {
      CHECK_RET(_validateDataByteAddress(activeChunkNo, locOffset));
    }
    uint16_t dAddr = __calcDataByteAddress(activeChunkNo, locOffset);
    uint16_t rAddr = dAddr + 1;

    uint8_t dByte, rByte;
    CHECK_RET(_readByte(dAddr, dByte));
    CHECK_RET(_readByte(rAddr, rByte));

    // [SAFETY CHECK/OPTIONAL?]
    if (!__areValidDAndRBytes(dByte, rByte)) {
      return E13_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_1;
    }

    if (locOffset == 0) {
      // case #1
      // First location - we must keep at least one non-empty (D-Byte or R-Byte) to keep marker that this Chunk is not Empty
      // However it also should be invalid - in case something goes wrong - so it will not be picked up as active value
      if (dByte == EMPTY_VALUE || rByte == EMPTY_VALUE) {
        // no need to erase anything (one of is already empty)
        // however we need to invalidate location
        if (dByte == EMPTY_VALUE) {
          
          CHECK_RET(_bitwiseAndByte(dAddr, 0xEE));
          CHECK_RET(_eraseByte(rAddr));
          
        } else {
          
          CHECK_RET(_bitwiseAndByte(rAddr, 0xEE));
          CHECK_RET(_eraseByte(dAddr));

        }
      } else {

        // erase R-Byte
        CHECK_RET(_eraseByte(rAddr));

      }

      // Double check : Validate that location is indeed invalid
      if (__SC_O) {
        uint8_t dByte, rByte;
        CHECK_RET(_readByte(dAddr, dByte));
        CHECK_RET(_readByte(rAddr, rByte));

        if (__areValidDAndRBytes(dByte, rByte)) {
          return E14_ERASE_ACTIVE_CHUNK_EEPROM_CORRUPTED_2;
        }
      }
      
    } else {

      // erase D-Byte
      if (dByte != EMPTY_VALUE) {
        CHECK_RET(_eraseByte(dAddr));
      }
      // erase R-Byte
      if (rByte != EMPTY_VALUE) {
        CHECK_RET(_eraseByte(rAddr));
      }
    }

    if (locOffset == 0) {
      break;
    } else {
      locOffset -= 2;
    }
  }

  return OK;
}

PrefOneByte::Error PrefOneByte::_eraseEveryFirstLocationOfEveryChunkGoingBackwards() {
  
  uint8_t chunkNo = _endChunk;
  while (true) {

    if (__SC_O) {
      CHECK_RET(_validateDataByteAddress(chunkNo, 0));
    }
    uint16_t dAddr = __calcDataByteAddress(chunkNo, 0);
    uint16_t rAddr = dAddr + 1;

    uint8_t dByte, rByte;
    CHECK_RET(_readByte(dAddr, dByte));
    CHECK_RET(_readByte(rAddr, rByte));

    // [SAFETY CHECK/OPTIONAL?] We expect location to be invalid (this is old location, not active)
    if (__areValidDAndRBytes(dByte, rByte)) {
      return E15_ERASE_WHOLE_CORRUPTED_1;
    }

    if (dByte == EMPTY_VALUE && rByte == EMPTY_VALUE) {
      // something strange here, should be invalid location, not empty
      return E16_ERASE_WHOLE_CORRUPTED_2;
      
    } else if (dByte != EMPTY_VALUE && rByte != EMPTY_VALUE) {
      // looks like some logic failed - we need to erase both bytes which is unexpected!
      return E17_ERASE_WHOLE_CORRUPTED_3;
      
    } else {
      
      // erase D-Byte
      if (dByte != EMPTY_VALUE) {
        CHECK_RET(_eraseByte(dAddr));
      }
      // erase R-Byte
      if (rByte != EMPTY_VALUE) {
        CHECK_RET(_eraseByte(rAddr));
      }
    }

    if (chunkNo == _startChunk) {
      break;
    } else {
      chunkNo--;
    }
  }

  return OK;
}

PrefOneByte::Error PrefOneByte::_save(const uint8_t newDataByte) {
  CHECK_RET(_preOperationCheck());
  
  bool isEEPROMEmpty;
  uint8_t activeChunkNo;
  uint8_t activeLocationOffset;
  CHECK_RET(_findActiveLocation(activeChunkNo, activeLocationOffset, isEEPROMEmpty));

  uint8_t activeDataByte = newDataByte;
  { // [SAFETY CHECK/MANDATORY] // Validate data in active location
    if (!isEEPROMEmpty) {
      CHECK_RET(_readAndValidateLocation(activeChunkNo, activeLocationOffset, activeDataByte));
    }
  }

  // Check if stored value is not equal to new value
  if (!isEEPROMEmpty && (activeDataByte == newDataByte)) {
    // No need to change anything, value is same
    return OK;
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

  if (__SC_O) {
    CHECK_RET(_validateDataByteAddress(targetChunkNo, targetLocationOffset));
  }
  uint16_t targetAddress = __calcDataByteAddress(targetChunkNo, targetLocationOffset);

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
    CHECK_RET(_eraseActiveChunkExceptFirstLocationRenderFirstLocationInvalidGoingBackward(activeChunkNo));
  }

  // step 11
  if (eraseWholeNeeded) {
    CHECK_RET(_eraseEveryFirstLocationOfEveryChunkGoingBackwards());
  }
  
  // step 2 (20 & 21)
  {
    { // [SAFETY CHECK/MANDATORY] Target location at this point must be empty!
      uint8_t targetDataByte, targetRedundancyByte;
      CHECK_RET(_readByte(targetAddress, targetDataByte));
      CHECK_RET(_readByte(targetAddress + 1, targetRedundancyByte));

      if (targetDataByte != EMPTY_VALUE || targetRedundancyByte != EMPTY_VALUE) {
        return E11_WRITE_ALG_ERR_1; // FIXME This is either EEPROM Corruption or ALG error
      }
    }
    
    if (newDataByte == 0x00 || newDataByte == 0xFF) {
      const uint16_t addr = newDataByte == 0x00 ? targetAddress : targetAddress + 1;
      CHECK_RET(_bitwiseAndByte(addr, 0x00));
    } else {
      CHECK_RET(_bitwiseAndByte(targetAddress, newDataByte));
      CHECK_RET(_bitwiseAndByte(targetAddress + 1, __calcRedundancyByte(newDataByte)));
    }
  }
  
  // SUCCESS
  return OK;
}

bool PrefOneByte::eraseStorage() {
  _lastError = _preOperationCheck();
  if (_lastError != OK) {
    return false;
  }
  
  for (uint8_t chunkNo = _startChunk; chunkNo <= _endChunk; chunkNo++) {
    uint16_t chunkAddress = chunkNo * _chunkSize;
    for (uint8_t j = 0; j < _chunkSize; j++) {
      _lastError = _eraseByteIfNotEmpty(chunkAddress + j);
      if (_lastError != OK) {
        return false;
      }
    }
  }
  return true;
}



uint8_t PrefOneByte::load() {
  if (!_loaded) {
     _lastError = _load(_isEmpty, _cachedDataByte);
    _loaded = _lastError == OK;
  }
  return _cachedDataByte;
}

bool PrefOneByte::isEmpty() {
  if (!_loaded) {
    load();
  }
  return _isEmpty;
}

bool PrefOneByte::save(const uint8_t dataByte) {
  _loaded = false; // mark to force re-load
  _lastError = _save(dataByte);
  return _lastError == OK;
}
