# Developer please read!
### Checklist (after you changed something)

 - [ ] Compile locally (create empty ../LongLiveThEEPROM.ino and try to compile it in Arduino IDE)
 - [ ] Deploy library locally (use ./deploy.bat)
 - [ ] Test every example (../examples/*) for compilation issues
 - [ ] Now check in emulation (./utest.sh - should be run from within Debian or WSL, see ./README.md)
 - [ ] Verify that library description matches your changes!

### Arduino library download logs:
  * [https://downloads.arduino.cc/libraries/logs/github.com/vmykhalchuk/LongLiveThEEPROM/]

### EEPROM Theory
  * Here Microchip answer confirming that smart handling of EEPROM can decrease its wear:
    - https://support.microchip.com/s/article/EEPROM---Queries-related-to-Endurance

### Promoting
  * Answer posts to promote:
    - https://arduino.stackexchange.com/questions/226/what-is-the-real-lifetime-of-eeprom
      - point to microchip article above, explain hot-spots (that each byte is a separate unit), explain that wear leveling also helps in future use of that chip because user will know for sure eeprom has no hot-spots. Suggest this library.