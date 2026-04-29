#ifndef UTEST_H
#define UTEST_H

#include <Arduino.h>
#include <EEPROM.h>

#define _printBeginningOfLine() Serial.print("|  ");

#define TERMINATE_MACRO \
          _printBeginningOfLine();                    \
          Serial.print(__FILE__);                     \
          Serial.print(":"); Serial.println(__LINE__);\
          UTest::dumpEEPROMToSerial();                \
          UTest::terminateSimulation(-1);             \
          while(1) {}

#ifndef TERMINATE_MACRO
  #error "TERMINATE_MACRO must be defined before using assert macroses! Add #define TERMINATE_MACRO at the top."
#endif

#define _assertEquals(expected, actual)               \
    if (expected != actual) {                         \
      _printBeginningOfLine();                        \
      Serial.print("Assertion Failed: ");             \
      Serial.print(". Expected 0x");                  \
      Serial.print(expected);                         \
      Serial.print(" but found 0x");                  \
      Serial.println(actual);                         \
      TERMINATE_MACRO;                                \
    }

#define _assertTrue(b) _assertEquals(true, b)
#define _assertFalse(b) _assertEquals(false, b)

/**
 * Checks if a specific EEPROM address contains the expected byte.
 */
#define _assertEEPROMByteEquals(address, expected) {    \
  uint8_t actual = EEPROM.read((uint16_t)address);      \
  if (actual != (uint8_t) expected) {                   \
    _printBeginningOfLine();                            \
    Serial.print("Assertion Failed: Address 0x");       \
    Serial.print(address, HEX);                         \
    Serial.print(" expected 0x");                       \
    Serial.print(expected, HEX);                        \
    Serial.print(" but found 0x");                      \
    Serial.println(actual, HEX);                        \
    TERMINATE_MACRO;                                    \
  }                                                     \
}

/**
 * Checks if a specific EEPROM address DOES NOT contain value
 */
#define _assertEEPROMByteNotEquals(address, value) {    \
  uint8_t actual = EEPROM.read((uint16_t)address);      \
  if (actual == (uint8_t) value) {                      \
    _printBeginningOfLine();                            \
    Serial.print("Assertion Failed: Address 0x");       \
    Serial.print(address, HEX);                         \
    Serial.print(" NOT expected 0x");                   \
    Serial.println(actual, HEX);                        \
    TERMINATE_MACRO;                                    \
  }                                                     \
}

/**
 * Checks if a specific EEPROM address is empty (0xFF)
 */
#define _assertEEPROMByteEmpty(address)             \
  _assertEEPROMByteEquals(address, 0xFF)

/**
 * Checks if a specific EEPROM address is NOT empty
 */
#define _assertEEPROMByteNotEmpty(address)          \
  _assertEEPROMByteNotEquals(address, 0xFF)

/**
 * Checks if every byte in a range contains the same specific value.
 */
#define _assertEEPROMRangeHasSameValue(addressStart,    \
              bytes, value) {                           \
  uint16_t addr = addressStart;                         \
  for (uint16_t i = 0; i < (uint8_t)bytes; i++) {       \
    uint8_t actual = EEPROM.read(addr + i);             \
    if (actual != (uint8_t) value) {                    \
      _printBeginningOfLine();                          \
      Serial.print("Assertion Failed: ");               \
      Serial.print("Range Error at 0x");                \
      Serial.print(addressStart + i, HEX);              \
      Serial.print(". Expected 0x");                    \
      Serial.print(value, HEX);                         \
      Serial.print(" but found 0x");                    \
      Serial.println(actual, HEX);                      \
      TERMINATE_MACRO;                                  \
    }                                                   \
  }                                                     \
}

/**
 * Checks if every byte in a range contains other but value
 */
#define _assertEEPROMRangeHasOtherButValue(addressStart,\
              bytes, value) {                           \
  uint16_t addr = addressStart;                         \
  for (uint16_t i = 0; i < (uint8_t)bytes; i++) {       \
    uint8_t actual = EEPROM.read(addr + i);             \
    if (actual == (uint8_t) value) {                    \
      _printBeginningOfLine();                          \
      Serial.print("Assertion Failed: ");               \
      Serial.print("Range Error at 0x");                \
      Serial.print(addressStart + i, HEX);              \
      Serial.print(". Expected <other>");               \
      Serial.print(" but found 0x");                    \
      Serial.println(actual, HEX);                      \
      TERMINATE_MACRO;                                  \
    }                                                   \
  }                                                     \
}

/**
 * Checks if a range of EEPROM memory is empty (0xFF).
 */
#define _assertEEPROMRangeEmpty(addressStart, bytes)    \
  _assertEEPROMRangeHasSameValue(addressStart, bytes, 0xFF)

/**
 * Checks if every byte in a range contains some value but empty (< 0xFF)
 */
#define _assertEEPROMRangeNotEmpty(addressStart, bytes) \
  _assertEEPROMRangeHasOtherButValue(addressStart, bytes, 0xFF)

class UTest {

  public:
    static void dumpEEPROMToSerial();
    static void dumpEEPROMToSerial(uint16_t limit);

    static void terminateSimAvrSimulation();

    // exitCode 0 - All OK
    static void terminateSimulation(int exitCode);

    static void eraseEEPROM();
};

#endif
