#include <pref_one_byte.h>

// first half of 1K EEPROM
PrefOneByte prefStorage1(32, 0, 15);
// second half of 1K EEPROM
PrefOneByte prefStorage2(32, 16, 31);

uint8_t pref1Byte;
uint8_t pref2Byte;

void setup() {
  Serial.begin(9600);

  // Validate if configuration is not broken
  if (prefStorage1.isConfigOK()) {
    Serial.println("Error in Config of Preferences Storage 1");
    while (1) {}; // halt if bad config
  }
  if (prefStorage2.isConfigOK()) {
    Serial.println("Error in Config of Preferences Storage 2");
    while (1) {}; // halt if bad config
  }

  // Load preferences
  pref1Byte = prefStorage1.load();
  Serial.print("Preferences #1 loaded: "); Serial.println(pref1Byte, HEX);
  pref2Byte = prefStorage2.load();
  Serial.print("Preferences #2 loaded: "); Serial.println(pref2Byte, HEX);

  // Modify preferences here
  if (prefStorage1.isEmpty()) { // it was empty
    pref1Byte = 0x10; // set to default config
  }
  
  // Modify preferences here
  if (prefStorage2.isEmpty()) { // it was empty
    pref2Byte = 0x20; // set to default config
  }

  // Save preferences
  prefStorage1.save(pref1Byte);
  prefStorage2.save(pref2Byte);
}

void loop() {
}
