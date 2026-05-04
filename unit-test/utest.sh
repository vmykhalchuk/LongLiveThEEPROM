#!/bin/bash

readonly BUILD_DIR="./build/"
readonly INO_BUILD_DIR="$BUILD_DIR/unit-test/"
readonly TARGET_DIR="$BUILD_DIR/target/"
readonly INO_FILE_NAME="unit-test.ino"

readonly COMPILE_FOR="arduino:avr:nano"
readonly MCU="atmega328p"
#arduino:avr:nano|atmega328p
#arduino:avr:nano:cpu=atmega168|atmega168
#arduino:avr:mega|atmega2560
#arduino:avr:mega:cpu=atmega1280|atmega1280

# Copy test files
rm -rf $BUILD_DIR
mkdir $BUILD_DIR
mkdir $INO_BUILD_DIR
mkdir $TARGET_DIR

cp ./$INO_FILE_NAME $INO_BUILD_DIR
cp ./*.h $INO_BUILD_DIR
cp ./*.cpp $INO_BUILD_DIR
cp ../*.h $INO_BUILD_DIR
cp ../*.cpp $INO_BUILD_DIR


# Compile
arduino-cli compile -v --fqbn $COMPILE_FOR --output-dir $TARGET_DIR $INO_BUILD_DIR
if [ $? -ne 0 ]; then
    echo "Error: arduino-cli compilation failed!"
    exit 1
fi

# Run simulation
./call_simavr.sh $TARGET_DIR/$INO_FILE_NAME.elf $BUILD_DIR/serial.out "@!@TEST_COMPLETE@!@" "@!@TEST_FAILED@!@" "$MCU"
if [ $? -ne 0 ]; then
    echo "Check error details: $BUILD_DIR/serial.out"
    exit 1
fi

