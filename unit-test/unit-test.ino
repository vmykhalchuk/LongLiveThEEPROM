#include <Arduino.h>
#include "eeprom_helper.h"
#include "pref_one_byte.h"

#include "utest.h"

void setup() {
  Serial.begin(9600);
  Serial.println("  Started!");

  // Test Cases

  Serial.println("  Testing basic assumptions");
  // Unit tests below assume chunk size = 32
  //_assertEquals(32, PrefOneByte::CHUNK_SIZE);
  // Unit tests below assume EEPROM size = 1024
  _assertEquals(1024, EEPROMHelper::EEPROM_SIZE);

  // 0) Happy test
  Serial.println("  testHappyScenario");
  testHappyScenario();
  UTest::dumpEEPROMToSerial(64);
  UTest::eraseEEPROM();

  Serial.println("  testSimplifiedMethods");
  testSimplifiedMethods();
  UTest::eraseEEPROM();

  Serial.println("  testSimplifiedMethods");
  testFaultyConfig();
  UTest::eraseEEPROM();

  // 1) Write first value as 0x00 - make sure byte0 is 0x00 and byte 1 is 0xFF
  //    now write 16 more bytes to ensure chunk overflow
  //    make sure byte 0 or byte 1 is empty while other is not empty and != 0x00 (aka this location is invalid)
  //    make sure other locations in chunk 0 are empty
  Serial.println("  testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00");
  testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00();
  UTest::eraseEEPROM();
  
  // 2) Write first value as 0x10 - run same steps and validations as in TC#1
  Serial.println("  testEveryFirstLocationInInactiveChunkIsInvalidAndAtLeastOneByteIsEmpty");
  testEveryFirstLocationInInactiveChunkIsInvalidAndAtLeastOneByteIsEmpty();
  UTest::eraseEEPROM();

  // 3) Write same value multiple times and make sure it is written only once
  Serial.println("  testWritingSameValueMultipleTimes");
  testWritingSameValueMultipleTimes();
  UTest::eraseEEPROM();

  Serial.println("  testWritingCornerValue(0x00)");
  testWritingCornerValue(0x00);
  UTest::eraseEEPROM();

  Serial.println("  testWritingCornerValue(0xFF)");
  testWritingCornerValue(0xFF);
  UTest::eraseEEPROM();

  // 4) Write value 512 times, make sure whole EEPROM is occupied
  Serial.println("  testWritingFull");
  testWritingFull();
  UTest::eraseEEPROM();

  // 5) Write value 515 times, make sure whole EEPROM is empty except first few locations in first chunk
  Serial.println("  testWritingAfterIsFull");
  testWritingAfterIsFull();
  UTest::eraseEEPROM();

  UTest::terminateSimulation(0); // SUCCESS
}

void testHappyScenario() {
  PrefOneByte config1;
  bool success;

  // Read and verify that EEPROM is empty
  uint8_t data = config1.load();
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(config1.isEmpty());

  // Write value as 0x11
  success = config1.save(0x11);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // Write value as 0x22
  success = config1.save(0x22);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // Read value back
  data = config1.load();
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x22, data);
  
}

void testSimplifiedMethods() {
  PrefOneByte config1;
  uint8_t conf = config1.load();
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(config1.isSuccess());
  _assertTrue(config1.isEmpty());
  _assertEquals(0xFF, conf);

  bool res = config1.save(0x11);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(config1.isSuccess());
  _assertTrue(res);
  
  conf = config1.load();
  _assertTrue(config1.isSuccess());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x11, conf);
}

void testFaultyConfig() {
  PrefOneByte pref1(63, 0, 123);
  uint8_t p = pref1.load();
  _assertFalse(pref1.isSuccess());
  _assertEquals(PrefOneByte::E01_INIT_ERROR, pref1.getLastError());
  _assertEquals(0xFF, p);

  pref1.save(0x01);
  _assertFalse(pref1.isSuccess());
  _assertEquals(PrefOneByte::E01_INIT_ERROR, pref1.getLastError());
}

void testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00() {
  PrefOneByte config1;
  bool success;
  success = config1.save(0x00);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);
  
  _assertEEPROMByteEquals(0, 0x00);
  _assertEEPROMByteEquals(1, 0xFF);

  for (uint8_t i = 1; i <= 16; i++) {
    success = config1.save(i);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
    _assertTrue(success);
  }

  _assertEEPROMByteEmpty(0);
  _assertEEPROMByteNotEmpty(1);
  
  _assertEEPROMRangeEmpty(2, 30);

  _assertEEPROMByteEquals(0x20, 16);
  _assertEEPROMByteEquals(0x21, ~16);
}

void testEveryFirstLocationInInactiveChunkIsInvalidAndAtLeastOneByteIsEmpty() {
  PrefOneByte config1;
  bool success;

  // will test first chunk
  testInactiveChunkFirstLocationIsInvalidatedWithItsValueEqual0x00();
  
  // now will test Second chunk

  for (uint8_t i = 1; i <= 16; i++) {
    success = config1.save(i);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
    _assertTrue(success);
  }

  _assertEEPROMByteNotEmpty(0x20);
  _assertEEPROMByteEmpty(0x21);
  
  _assertEEPROMRangeEmpty(0x22, 30);

  _assertEEPROMByteEquals(0x40, 16);
  _assertEEPROMByteEquals(0x41, ~16);


  // now will test Third chunk

  for (uint8_t i = 1; i <= 16; i++) {
    success = config1.save(i);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
    _assertTrue(success);
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
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // repeat writing same value 5 more times
  for (int i = 0; i < 5; i++) {
    success = config1.save(0x77);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
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
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  success = config1.save(0x22);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // Write <cornerValue>
  success = config1.save(cornerValue);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // write another value and check if its not <cornerValue> by any chance
  success = config1.save(0x23);
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertTrue(success);

  // test last stored value
  // Might be that corner value breaks logic
  uint8_t data = config1.load();
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertFalse(config1.isEmpty());
  _assertEquals(0x23, data);
}

void testWritingFull() {
  PrefOneByte config1;
  bool success;

  // write whole EEPROM of ram
  for (int i = 0; i < 512; i++) {
    success = config1.save(i % 64);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
    _assertTrue(success);
  }

  // assert read succeeds
  uint8_t data = config1.load();
    _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertFalse(config1.isEmpty());
  _assertEquals((511 % 64), data);
  
  // FIXME Test memory
  //UTest::dumpEEPROMToSerial();

  _assertEEPROMRangeEmpty((1024 - 64*3 + 2), (32 - 2));
  _assertEEPROMRangeEmpty((1024 - 64*2 + 2), (32 - 2));
  _assertEEPROMRangeEmpty((1024 - 64 + 2),   (32 - 2));
  _assertEEPROMRangeNotEmpty((1024 - 32), 32);
}

void testWritingAfterIsFull() {
  PrefOneByte config1;
  // write full
  testWritingFull();

  bool success;
  
  // write few times
  for (int i = 1; i <= 3; i++) {
    success = config1.save(30+i);
    _assertEquals(PrefOneByte::OK, config1.getLastError());
    _assertTrue(success);
  }

  // assert read succeeds
  uint8_t data = config1.load();
  _assertEquals(PrefOneByte::OK, config1.getLastError());
  _assertFalse(config1.isEmpty());
  _assertEquals(33, data);

  // assert EEPROM is empty
  _assertEEPROMRangeEmpty(0x06, (1024 - 6));
}

void loop() {
}
