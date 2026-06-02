#!/bin/bash

readonly BUILD_DIR="./build/"
readonly INO_BUILD_DIR="$BUILD_DIR/unit-test/"
readonly TARGET_DIR="$BUILD_DIR/target/"
readonly INO_FILE_NAME="unit-test.ino"

echo "------------------$1-------------------"

case "$1" in
    "nano")
        readonly COMPILE_FOR="arduino:avr:nano"
        readonly MCU="atmega328p"
	;;
    "nano-old")
        readonly COMPILE_FOR="arduino:avr:nano:cpu=atmega168"
        readonly MCU="atmega168"
        ;;
    "mega")
        readonly COMPILE_FOR="arduino:avr:mega"
        readonly MCU="atmega2560"
        ;;
    "mega-old")
        readonly COMPILE_FOR="arduino:avr:mega:cpu=atmega1280"
        readonly MCU="atmega1280"
        ;;
    *)
        readonly COMPILE_FOR="arduino:avr:nano"
        readonly MCU="atmega328p"
	;;
esac

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

readonly SERIAL_OUT="$BUILD_DIR/serial.out"

# Compile
arduino-cli compile -v --fqbn $COMPILE_FOR --output-dir $TARGET_DIR $INO_BUILD_DIR
if [ $? -ne 0 ]; then
    echo "Error: arduino-cli compilation failed!"
    exit 1
fi

# Run simulation
./call_simavr.sh $TARGET_DIR/$INO_FILE_NAME.elf $SERIAL_OUT "@!@TEST_COMPLETE@!@" "@!@TEST_FAILED@!@" "$MCU"
if [ $? -ne 0 ]; then
    echo "Check error details: $SERIAL_OUT"
    exit 1
else
    cat $SERIAL_OUT
fi

