#include <pref_one_byte.h>

// [SAFETY CHECKS]
// It is possible to control how much Sanity/Safety Checks are performed by library
// This is done by using different classes:
//  - PrefOneByte - DEFAULT one - LVL 2 is used (Optional check included to safely detect corrupted EEPROM)
//  - PrefOneByteFeather - No Optional checks, bare minimum (Only Redundancy check - still enough for most use cases)
//  - PrefOneByteParanoia - Absolutely max checks (before every erase/write)
// NOTE: The more checks - the more code space is used, also slightly slower code execution

// [CONFIGURATION]
// By default whole EEPROM space is utilized.
// This can be changed to limit area used by storage, and define separate area for different storages.
// For this use parameterized constructor:
//  - PrefOneByte(chunkSize, startChunk, endChunk)
//        chunkSize should be odd number and greater than 8
//        startChunk - to offset from beginning of EEPROM, make it other than 0
//        endChunk - last chunk to use for this Storage
// NOTE: Make sure EEPROM Area is empty and does not contain garbage - otherwise library will fail to work

// [ERASING]
// There are two ways to erase:
//   - preferences1.eraseStorage() - best solution - it will make sure that area of EEPROM designated for this particular storage is erased
//   - EEPROMHelper::nukeWholeEEPROM() - as name suggests - it will erase every single byte of EEPROM watchout for it!

// first half of 1K EEPROM
PrefOneByteFeather preferences1(32, 0, 15);
// second half of 1K EEPROM
PrefOneByteFeather preferences2(32, 16, 31);

uint8_t pref1Byte;
uint8_t pref2Byte;

void setup() {
  Serial.begin(9600);

  // Validate if configuration is not broken
  if (preferences1.isConfigOK()) {
    Serial.println("Error in Config of Preferences 1");
    while (1) {}; // halt if bad config
  }
  if (preferences2.isConfigOK()) {
    Serial.println("Error in Config of Preferences 2");
    while (1) {}; // halt if bad config
  }

  // Load preferences
  pref1Byte = preferences1.load();
  Serial.print("Preferences #1 loaded: "); Serial.println(pref1Byte, HEX);
  pref2Byte = preferences2.load();
  Serial.print("Preferences #2 loaded: "); Serial.println(pref2Byte, HEX);

  // Modify preferences here
  if (preferences1.isEmpty()) { // it was empty
    pref1Byte = 0x10; // set to default config
  }
  
  // Modify preferences here
  if (preferences2.isEmpty()) { // it was empty
    pref2Byte = 0x20; // set to default config
  }

  // Save preferences
  preferences1.save(pref1Byte);
  preferences2.save(pref2Byte);
}

void loop() {
}
