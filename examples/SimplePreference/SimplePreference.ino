#include <pref_one_byte.h>

PrefOneByte prefStorage;
uint8_t prefByte;

void setup() {
  Serial.begin(9600);
  prefByte = prefStorage.load();
  Serial.print("Preferences loaded: "); Serial.println(prefByte, HEX);

  // modify preferences here
  if (prefStorage.isEmpty()) { // Pref Storage is empty
    prefByte = 0x10; // set to default config
  }
  
  prefStorage.save(prefByte);
}

void loop() {
}
