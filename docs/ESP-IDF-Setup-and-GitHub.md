# ESP-IDF Setup and GitHub Workflow

This guide is for reviewers running the project on a clean Linux machine.

## 1) Install system packages

Tested on Ubuntu 20.04.

```bash
sudo apt update
sudo apt install -y git wget flex bison gperf cmake ninja-build ccache \
	libffi-dev libssl-dev dfu-util libusb-1.0-0 python3 python3-pip python3-venv
```

## 2) Install ESP-IDF

```bash
mkdir -p $HOME/esp
cd $HOME/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout release/v5.3
git submodule update --init --recursive
./install.sh
```

## 3) Load ESP-IDF environment

For current shell:

```bash
. $HOME/esp/esp-idf/export.sh
```

Check tools:

```bash
idf.py --version
python --version
```

Optional shell helper (zsh):

```bash
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.zshrc
```

## 4) Configure USB serial access

```bash
sudo usermod -aG dialout $USER
```

Then log out and log in again.

Typical device names are `/dev/ttyUSB0` or `/dev/ttyACM0`.

## 5) Clone project from GitHub

HTTPS:

```bash
git clone https://github.com/Gujenov/test-task-led.git
cd test-task-led
```

SSH (if your key is already configured):

```bash
git clone git@github.com:Gujenov/test-task-led.git
cd test-task-led
```

## 6) Build, flash, monitor

Always load ESP-IDF environment first:

```bash
. $HOME/esp/esp-idf/export.sh
```

Then run:

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

## 7) Typical GitHub workflow (for contributors)

```bash
git checkout -b my-change
# edit files
git add .
git commit -m "Describe your change"
git push -u origin my-change
```

Then create a Pull Request on GitHub.

## 8) Troubleshooting

- `idf.py: command not found`:
	- environment is not loaded, run `. $HOME/esp/esp-idf/export.sh`.
- Serial port busy:
	- close monitor in another terminal, then retry flash.
- Permission denied on serial port:
	- check `dialout` group and relogin.
- Wrong target after previous builds:
	- run `idf.py set-target esp32` again.

## Official references

- ESP-IDF Programming Guide:
	- https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32/
- ESP-IDF Get Started:
	- https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32/get-started/
- esptool documentation:
	- https://docs.espressif.com/projects/esptool/en/latest/esp32/
