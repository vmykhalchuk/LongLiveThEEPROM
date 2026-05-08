
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

### Run Unit Tests
  - checkout repo
  - navigate into this directory
  - run utest-all.sh
    - tip: use `chmod +x call_simavr.sh && chmod +x utest.sh && chmod +x utest-all.sh` to make it executable

### Setup WSL on Windows 11
  - Install WSL
    - wsl --install
  - Configure WSL
    - wsl --set-default-version 2
  - Install Debian
    - wsl --install -d Debian
  - Upgrade if needed to run on WSL 2
    - check first if its 1 or 2
      - wsl -l -v
    - wsl --set-version debian 2
  - Extra
    - wsl --set-default Debian

### Install library from local source
  - Use deploy.bat
  - Or step by step:
    - Navigate to parent dir of LongLiveThEEPROM
    - Create zip
      * tar -a -c -f LongLiveThEEPROM.zip LongLiveThEEPROM
    - Install library
      * arduino-cli lib install --zip-path LongLiveThEEPROM.zip
      * If Error: --git-url and --zip-path are disabled by default, for more information see: https://arduino.github.io/arduino-cli/1.4/configuration/#configuration-keys
        * arduino-cli config set library.enable_unsafe_install true
