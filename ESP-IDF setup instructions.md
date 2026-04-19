ESP-IDF setup notes for Ubuntu 20.04

This is a short, practical setup flow for ESP-IDF 5.3.x on Ubuntu 20.04. The goal is simple: get from a clean system to a working build and flash enviroment without too much fuss.

1. Install base packages

Run:

sudo apt update
sudo apt install -y git wget flex bison gperf cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 python3 python3-pip python3-venv

Ubuntu 20.04 usually gives you Python 3.8, which is fine for ESP-IDF 5.3.x.


2. Check Python

Run:

python3 --version
python3 -m pip --version

If pip looks broken, you can do:

python3 -m pip install --upgrade pip

ESP-IDF will still create its own Python virtual env later, but the system Python needs to be present and working.


3. Clone ESP-IDF

mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout release/v5.3
git submodule update --init --recursive


4. Install ESP-IDF tools

From inside ~/esp/esp-idf:

./install.sh

This normally puts toolchains and Python envs under ~/.espressif. First run can take a bit, so just let it complete proeprly.


5. Load the environment

. ~/esp/esp-idf/export.sh

After that, these should work:

idf.py --version
python --version

If `idf.py` is not found, export.sh probably was not sourced in the current shell.


6. Verify with a real build

cd ~/esp/esp-idf/examples/get-started/hello_world
idf.py set-target esp32
idf.py build

If that builds, the setup is basically okay.


7. Enable serial port access

For flashing, add your user to dialout:

sudo usermod -aG dialout $USER

Then log out and back in. When a board is connected, it will usually appear as `/dev/ttyUSB0` or `/dev/ttyACM0`.


8. Flash a board

idf.py -p /dev/ttyUSB0 flash monitor

Change the port if needed. If flashing fails, check permissions, the USB cable, and whether the board is actually detected.


9. Handy optional alias

For zsh:

echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.zshrc



Main folders you will usually end up with:

- `~/esp/esp-idf` — the ESP-IDF repo
- `~/.espressif/tools` — toolchains and helper tools
- `~/.espressif/python_env` — ESP-IDF Python virtual environments

That is pretty much the whole setup. Short version: install packages, make sure Python exists, clone ESP-IDF, run `install.sh`, source `export.sh`, build `hello_world`, then check serial access for flashing.
