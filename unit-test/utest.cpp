#include "utest.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

void UTest::dumpEEPROMToSerial() {
  dumpEEPROMToSerial(E2END + 1);
}

void UTest::dumpEEPROMToSerial(uint16_t limit) {
  limit = limit >= E2END ? E2END + 1 : limit;
  _printBeginningOfLine();
  Serial.println("--- EEPROM DUMP START ---");
  _printBeginningOfLine();
  for (uint16_t i = 0; i < limit; i++) {
    byte value = EEPROM.read(i);
    if (value < 0x10) Serial.print("0"); // Leading zero for hex
    Serial.print(value, HEX);
    Serial.print(" ");
    if ((i + 1) % 32 == 0) {
      Serial.println(); // New line every 32 bytes
      _printBeginningOfLine();
    }
  }
  Serial.println("--- EEPROM DUMP END ---");
}

void UTest::terminateSimAvrSimulation() {
  // FIXME Doesn't work - investigate why
  cli();                // Disable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();          // Simulator sees interrupts are off + sleep = Exit
}

void UTest::terminateSimulation(int exitCode) {
  Serial.println();
  if (exitCode == 0) {
    Serial.println("@!@TEST_COMPLETE@!@");
  } else {
    _printBeginningOfLine();
    Serial.print("Terminating with Error: "); Serial.println(exitCode);
    Serial.println("@!@TEST_FAILED@!@");
  }
  Serial.flush();

  terminateSimAvrSimulation();
  while (1) {};
}

/**
 * Erases whole EEPROM
 */
void UTest::eraseEEPROM() {
  for (uint16_t addr = 0; addr <= E2END; addr++) {
    EEPROM.update(addr, 0xFF);
  }
}
