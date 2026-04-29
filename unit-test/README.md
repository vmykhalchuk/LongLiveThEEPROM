
## Run tests on Debian (or WSL for Windows)
### Install all required tools

  - install essentials
    sudo apt update && sudo apt upgrade -y
    sudo apt install git build-essential libelf-dev avr-libc gcc-avr gtkwave

    sudo apt install pkg-config freeglut3-dev libusb-1.0-0-dev

  - install simavr
    cd ~
    git clone https://github.com/buserror/simavr.git
    cd simavr
    make
    sudo make install

  - install arduino-cli
    cd ~
    sudo apt install curl
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    sudo mv ./bin/arduino-cli /usr/local/bin/
    arduino-cli version

### Run tests
  - checkout repo
  - navigate into this directory
  - run utest.sh
    - tip: use `chmod +x call_simavr.sh && chmod +x utest.sh` to make it executable

## Install library from local source
  - Navigate to dir where LongLiveThEEPROM located
  - Create zip
    * tar -a -c -f LongLiveThEEPROM.zip LongLiveThEEPROM
  - Install library
    * arduino-cli lib install --zip-path LongLiveThEEPROM.zip
    * If Error: --git-url and --zip-path are disabled by default, for more information see: https://arduino.github.io/arduino-cli/1.4/configuration/#configuration-keys
      * arduino-cli config set library.enable_unsafe_install true
