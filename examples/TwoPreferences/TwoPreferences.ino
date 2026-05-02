#include <pref_one_byte.h>

PrefOneByte::ConfigError pref1ConfigError;
PrefOneByte::ConfigError pref2ConfigError;

// first half of 1K EEPROM
PrefOneByte preferences1(32, 0, 15, pref1ConfigError);
// second half of 1K EEPROM
PrefOneByte preferences2(32, 16, 31, pref2ConfigError);

uint8_t pref1Byte;
uint8_t pref2Byte;

void setup() {
  Serial.begin(9600);

  // Validate if configuration is not broken
  if (pref1ConfigError != PrefOneByte::CONFIG_OK) {
    Serial.println("Error in Config of Preferences 1");
    while (1) {}; // halt if bad config
  }
  if (pref2ConfigError != PrefOneByte::CONFIG_OK) {
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
