#include <pref_one_byte.h>

PrefOneByte preferences;
uint8_t prefByte;

void setup() {
  Serial.begin(9600);
  prefByte = preferences.load();
  Serial.print("Preferences loaded: "); Serial.println(prefByte, HEX);

  // modify preferences here
  if (preferences.isEmpty()) { // Pref Storage is empty
    prefByte = 0x10; // set to default config
  }
  
  preferences.save(prefByte);
}

void loop() {
}
