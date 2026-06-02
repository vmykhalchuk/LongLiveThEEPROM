/*

    ****** ATTENTION ATTENTION ATTENTION ******
    ***** THIS SKETCH WILL EXHAUST EEPROM *****
    ***** PREFER IT RUNNING IN EMULATOR ******

*/
#include <Arduino.h>
#include "eeprom_helper.h"
#include "pref_one_byte.h"

#include "utest.h"

#define RUN_TEST_CASE(name) \
    do { \
        Serial.print("  "); \
        Serial.println(#name); \
        test##name(); \
        UTest::eraseEEPROM(); \
    } while (0)

#define RUN_TEST_CASE_PAR1(name, par1) \
    do { \
        Serial.print("  "); \
        Serial.print(#name); \
        Serial.print("("); \
        Serial.print(par1); \
        Serial.println(")"); \
        test##name(par1); \
        UTest::eraseEEPROM(); \
    } while (0)
    

#define RUN_TEST_CASE_DUMP(name, dumpBytes) \
    do { \
        Serial.print("  "); \
        Serial.println(#name); \
        test##name(); \
        UTest::dumpEEPROMToSerial(dumpBytes); \
        UTest::eraseEEPROM(); \
    } while (0)

void setup() {
  Serial.begin(9600);
  Serial.println("  Started!");
  Serial.print("   EEPROM Size: "); Serial.println(EEPROMHelper::EEPROM_SIZE);

  // Test Cases

  Serial.println("  Testing basic assumptions");
  // Unit tests below assume EEPROM size >= 256
  _assertTrue(EEPROMHelper::EEPROM_SIZE >= 256);

  // 0) Happy test
  RUN_TEST_CASE_DUMP(HappyScenario, 64);
  
  RUN_TEST_CASE(SimplifiedMethods);
  RUN_TEST_CASE(SimplifiedMethods_IsSuccess);
  RUN_TEST_CASE(SimplifiedMethods_IsEmpty);
  RUN_TEST_CASE(SimplifiedMethods_LoadCaching);
  RUN_TEST_CASE(SimplifiedMethods_LoadAfterSave);

  RUN_TEST_CASE(FaultyConfig);

  // 1) Write first value as 0x00 - make sure byte0 is 0x00 and byte 1 is 0xFF
  //    now write 16 more bytes to ensure chunk overflow
  //    make sure byte 0 or byte 1 is empty while other is not empty and != 0x00 (aka this location is invalid)
  //    make sure other locations in chunk 0 are empty
  RUN_TEST_CASE(InactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00);
  
  // 2) Write first value as 0x10 - run same steps and validations as in TC#1
  RUN_TEST_CASE(EveryFirstLocationInInactiveChunkIsInvalidAndAtLeastOneByteIsEmpty);

  // 3) Write same value multiple times and make sure it is written only once
  RUN_TEST_CASE(WritingSameValueMultipleTimes);
  RUN_TEST_CASE_PAR1(WritingCornerValue, 0x00);
  RUN_TEST_CASE_PAR1(WritingCornerValue, 0xFF);

  // 4) Write value 512 times, make sure whole EEPROM is occupied
  RUN_TEST_CASE(WritingFull);

  // 5) Write value 515 times, make sure whole EEPROM is empty except first few locations in first chunk
  RUN_TEST_CASE(WritingAfterIsFull);

  UTest::terminateSimulation(0); // SUCCESS
}

void testHappyScenario() {
  PrefOneByte config1;

  // Read and verify that EEPROM is empty
  uint8_t data = config1.load();
  _assertTrue(config1.isSuccess());
  _assertTrue(config1.isEmpty());

  // Write value as 0x11
  _assertTrue(config1.save(0x11));

  // Write value as 0x22
  _assertTrue(config1.save(0x22));

  // Read value back
  data = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x22, data);
}

void testSimplifiedMethods() {
  PrefOneByte config1;
  uint8_t dataByte = config1.load();
  _assertTrue(config1.isSuccess());
  _assertTrue(config1.isEmpty());
  _assertEquals(0xFF, dataByte);

  _assertTrue(config1.save(0x12));
  
  dataByte = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x12, dataByte);
}

void testSimplifiedMethods_IsSuccess() {
  PrefOneByte config1;
  config1.load();
  _assertEquals(PREF_ONE_BYTE::OK, config1.getLastError());
  _assertTrue(config1.isSuccess()); // Success is directly coupled to lastError
}

void testSimplifiedMethods_IsEmpty() {
  PrefOneByte config1;
  bool _empty = config1.isEmpty();
  _assertTrue(_empty);
  _assertTrue(config1.isSuccess());
  _assertTrue(config1.isEmpty());
}

void testSimplifiedMethods_LoadCaching() {
  PrefOneByte config1;
  _assertTrue(config1.save(0x23));

  uint8_t dataByte = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x23, dataByte);

  // erase EEPROM
  UTest::eraseEEPROM();
  
  dataByte = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x23, dataByte); // make sure cached value returns
}

void testSimplifiedMethods_LoadAfterSave() {
  PrefOneByte config1;
  _assertTrue(config1.save(0x34));

  // make sure fresh value is loaded (this test can easily break when save method changes behavior)
  uint8_t dataByte = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x34, dataByte);
}

void testFaultyConfig() {
  PrefOneByte pref1(254, 0, 255);

  uint8_t p = pref1.load();
  _assertFalse(pref1.isSuccess());
  _assertEquals(PREF_ONE_BYTE::E01_CONFIG_ERROR, pref1.getLastError());
  _assertEquals(0xFF, p);

  bool res = pref1.save(0x01);
  _assertFalse(res);
  _assertFalse(pref1.isSuccess());
  _assertEquals(PREF_ONE_BYTE::E01_CONFIG_ERROR, pref1.getLastError());

  res = pref1.eraseStorage();
  _assertFalse(res);
  _assertFalse(pref1.isSuccess());
  _assertEquals(PREF_ONE_BYTE::E01_CONFIG_ERROR, pref1.getLastError());
}

void testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00() {
  PrefOneByte config1(32, 0, 7); // we limit to first 256 bytes
  _assertTrue(config1.save(0x00));
  
  _assertEEPROMByteEquals(0, 0x00);
  _assertEEPROMByteEquals(1, 0xFF);

  for (uint8_t i = 1; i <= 16; i++) {
    _assertTrue(config1.save(i));
  }

  _assertEEPROMByteEmpty(0);
  _assertEEPROMByteNotEmpty(1);
  
  _assertEEPROMRangeEmpty(2, 30);

  _assertEEPROMByteEquals(0x20, 16);
  _assertEEPROMByteEquals(0x21, ~16);
}

void testEveryFirstLocationInInactiveChunkIsInvalidAndAtLeastOneByteIsEmpty() {
  PrefOneByte config1(32, 0, 7); // we limit to first 256 bytes

  // will test first chunk
  testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00();
  
  // now will test Second chunk

  for (uint8_t i = 1; i <= 16; i++) {
    _assertTrue(config1.save(i));
  }

  _assertEEPROMByteNotEmpty(0x20);
  _assertEEPROMByteEmpty(0x21);
  
  _assertEEPROMRangeEmpty(0x22, 30);

  _assertEEPROMByteEquals(0x40, 16);
  _assertEEPROMByteEquals(0x41, ~16);


  // now will test Third chunk

  for (uint8_t i = 1; i <= 16; i++) {
    _assertTrue(config1.save(i));
  }

  _assertEEPROMByteNotEmpty(0x40);
  _assertEEPROMByteEmpty(0x41);
  
  _assertEEPROMRangeEmpty(0x42, 30);

  _assertEEPROMByteEquals(0x60, 16);
  _assertEEPROMByteEquals(0x61, ~16);
}

void testWritingSameValueMultipleTimes() {
  PrefOneByte config1;
  // prepare
  testHappyScenario();

  bool success;

  // Write value as 0x77
  success = config1.save(0x77);
  _assertTrue(config1.isSuccess());
  _assertTrue(success);

  // repeat writing same value 5 more times
  for (int i = 0; i < 5; i++) {
    success = config1.save(0x77);
    _assertTrue(config1.isSuccess());
    _assertTrue(success);
  }

  // now check memory
  _assertEEPROMByteEquals(0x00,  0x11);
  _assertEEPROMByteEquals(0x01, ~0x11);
  _assertEEPROMByteEquals(0x02,  0x22);
  _assertEEPROMByteEquals(0x03, ~0x22);
  _assertEEPROMByteEquals(0x04,  0x77);
  _assertEEPROMByteEquals(0x05, ~0x77);
  // rest should be empty
  _assertEEPROMRangeEmpty(0x06, 100);
  
}

// test 0x00 and 0xFF
void testWritingCornerValue(uint8_t cornerValue) {
  PrefOneByte config1;
  // Makes sure writing corner value doesn't break retrieve logic
  bool success;
  
  success = config1.save(0x21);
  _assertTrue(config1.isSuccess());
  _assertTrue(success);

  success = config1.save(0x22);
  _assertTrue(config1.isSuccess());
  _assertTrue(success);

  // Write <cornerValue>
  success = config1.save(cornerValue);
  _assertTrue(config1.isSuccess());
  _assertTrue(success);

  // write another value and check if its not <cornerValue> by any chance
  success = config1.save(0x23);
  _assertTrue(config1.isSuccess());
  _assertTrue(success);

  // test last stored value
  // Might be that corner value breaks logic
  uint8_t data = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x23, data);
}

void testWritingFull() {
   // we limit to first 256 bytes (also avoiding first two chunks)
  constexpr uint16_t eeSize = 256;
  constexpr uint8_t chunkSize = 32;
  constexpr uint8_t startChunk = 2;
  constexpr uint8_t endChunk = 7;
  PrefOneByte config1(chunkSize, startChunk, endChunk);

  // write whole storage area (128 locations)
  constexpr uint16_t maxLocationsCount = chunkSize / 2 * (endChunk - startChunk + 1);
  for (uint16_t i = 0; i < maxLocationsCount; i++) {
    _assertTrue(config1.save(i % 64));
  }

  // assert read succeeds
  uint8_t data = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(((maxLocationsCount - 1) % 64), data);

  // Test working area
  _assertEEPROMRangeEmpty((eeSize - chunkSize*4 + 2), (chunkSize - 2));
  _assertEEPROMRangeEmpty((eeSize - chunkSize*3 + 2), (chunkSize - 2));
  _assertEEPROMRangeEmpty((eeSize - chunkSize*2 + 2), (chunkSize - 2));
  //_assertEEPROMRangeEmpty((eeSize - chunkSize*1 + 2),   (chunkSize - 2));
  _assertEEPROMRangeNotEmpty((eeSize - chunkSize), chunkSize);

  // Assert that remainding EEPROM area is empty (is untouched by library code)
  _assertEEPROMRangeEmpty(0, chunkSize * startChunk);
  _assertEEPROMRangeEmpty(eeSize, EEPROMHelper::EEPROM_SIZE - eeSize);
}

void testWritingAfterIsFull() {
   // we limit to first 256 bytes (also avoiding first two chunks)
  constexpr uint16_t eeSize = 256;
  constexpr uint8_t chunkSize = 32;
  constexpr uint8_t startChunk = 2;
  constexpr uint8_t endChunk = 7;
  PrefOneByte config1(chunkSize, startChunk, endChunk);
  
  // write full
  testWritingFull();

  // write few times
  for (uint16_t i = 1; i <= 3; i++) {
    _assertTrue(config1.save(30+i));
  }

  // assert read succeeds
  uint8_t data = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(33, data);

  // assert Storage Area is empty
  _assertEEPROMRangeEmpty(startChunk * chunkSize + 0x06, (eeSize - 6));

  // Assert that remainding EEPROM area is empty (is untouched by library code)
  _assertEEPROMRangeEmpty(0, chunkSize * startChunk);
  _assertEEPROMRangeEmpty(eeSize, EEPROMHelper::EEPROM_SIZE - eeSize);
}

void loop() {
}
