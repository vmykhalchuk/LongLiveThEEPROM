## EISENHOWER MATRIX

### URGENT: IMPORTANT
  
  * Modify to store not single but flexible number of bytes

### NOT URGENT : IMPORTANT
 
  * Add Examples
    * Simple example to configure 1 Byte storage only
    * Example to configure multiple 1 Byte storages
    * Example to clean-up EEPROM (when configuration change is needed)

  * Fix simavr bug where it doesn't erase byte to 0xFF instead it writes value stored in EEDR register
    * it is handled by https://github.com/buserror/simavr/blob/master/simavr/sim/avr_eeprom.c
    * thing is that simavr doesn't care if its erase/write, erase or write operation mode. All modes are treated as erase/write.
    * see code: if (eempe && avr_regbit_get(avr, p->eepe)) {	// write operation
       it always takes 3.4ms, see delay below

### URGENT: NOT IMPORTANT


### NOT URGENT : NOT IMPORTANT

  * Consider other name for PerfOneByte (ConfigStoreEEPROM)
  * PERF OPTIM: When advancing to next location - invalidate previous one
       this will reduce number of erase instructions required to clean active chunk

  * Implement Unit Testing for Bare metal
    * replace tv_eeprom.cpp with dummy implementation in RAM
    * output test results to Serial
