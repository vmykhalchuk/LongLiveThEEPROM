cd ../..
set LIB_NAME=LongLiveThEEPROM
tar -a -c -f %LIB_NAME%.zip -X %LIB_NAME%/.unit-test/deploy.exclude.txt %LIB_NAME%
arduino-cli lib install --zip-path %LIB_NAME%.zip
pause