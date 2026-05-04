# LongLiveThEEPROM

Maximize Arduino EEPROM lifespan through wear-leveling and data validation.

Standard EEPROM is rated for ~100K writes. LongLiveThEEPROM implements a Ring Buffer strategy
that distributes writes across the available memory. When tracking a single byte on an Arduino Nano,
this can extend life expectancy by up to 512x (effectively 51.2M writes).

### Key Features:

* Wear-Leveling: Prevents hardware failure by spreading write cycles.

* Data Integrity: Integrated validation ensures your app gets notified when EEPROM fails.

Validation is implemented by storing inverted redundancy value - this maximizes chances to
detect EEPROM bit-rot or 'stuck-bits'.

### Simple usage

``` C++
#include <pref_one_byte.h>

PrefOneByte preferences;

uint8_t prefByte;

void setup() {
  Serial.begin(9600);

  // Load preferences byte from EEPROM
  prefByte = preferences.load();

  Serial.print("Preferences loaded: "); Serial.println(prefByte, HEX);

  // modify preferences here
  if (preferences.isEmpty()) { // EEPROM is empty (was never written to)
    prefByte = 0x10; // set to default config
  }
  
  // Save preferences byte to EEPROM
  preferences.save(prefByte);
}

void loop() {
}

```