## EISENHOWER MATRIX

### URGENT: IMPORTANT

  * Remove support of AVR Boards that do not have EEPROM (currently it is still shown in arduino docs libraries)
  * Add unit tests covering all AVR MCUs
  * Modify to store not single but configurable number of bytes
    - why urgent: make it more flexible and useful

### URGENT: NOT IMPORTANT


### NOT URGENT : IMPORTANT


  * Add possibility for user to create two/four preferences with simple calls (current TwoPreferences example is unclear for novice user)
  * Add constructor to let user define startOffset vs startChunk to make configuration more flexible

  * Add Examples
    * Example to clean-up EEPROM (when configuration change is needed)
    * Add example on how to migrate storage changes (e.g. 1 area => 2 areas and keep first in first or second; or opposite 2 => 1 and do some compaction 2 bytes into 1, etc)

  * PERF OPTIM: When advancing to next location - invalidate previous one
       this will reduce number of erase instructions required to clean active chunk

  * Fix simavr bug where it doesn't erase byte to 0xFF instead it writes value stored in EEDR register
    * it is handled by https://github.com/buserror/simavr/blob/master/simavr/sim/avr_eeprom.c
    * thing is that simavr doesn't care if its erase/write, erase or write operation mode. All modes are treated as erase/write.
    * see code: if (eempe && avr_regbit_get(avr, p->eepe)) {	// write operation
       it always takes 3.4ms, see delay below


### NOT URGENT : NOT IMPORTANT

  * Add validation to check if registered Pref storages do not overlap
  * Make compiler use static resolution at comile time to config pref storages (to avoid use of RAM)

  * Consider other name for PrefOneByte (ConfigStoreEEPROM)

  * Implement Unit Testing for Bare metal
    * replace eeprom_helper.cpp with dummy implementation in RAM
    * output test results to Serial
