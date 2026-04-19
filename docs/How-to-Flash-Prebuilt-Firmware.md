# How to Flash Prebuilt Firmware (ESP32)

This guide lets you flash firmware binaries from this repository without running `idf.py build`.

## Files used

Prebuilt bundle location:
- `prebuilt/esp32/bootloader.bin`
- `prebuilt/esp32/partition-table.bin`
- `prebuilt/esp32/test-task-led.bin`
- `prebuilt/esp32/flash_args`
- `prebuilt/esp32/SHA256SUMS.txt`

## 1) Install prerequisites

You need Python 3 and esptool.

Install esptool:

```bash
python3 -m pip install --user esptool
```

## 2) Identify serial port

Typical ports on Linux:
- `/dev/ttyUSB0`
- `/dev/ttyACM0`

## 3) Optional: verify checksums

```bash
cd prebuilt/esp32
sha256sum -c SHA256SUMS.txt
```

All lines should report `OK`.

## 4) Flash the prebuilt files

```bash
cd prebuilt/esp32
python3 -m esptool --chip esp32 -p /dev/ttyUSB0 -b 460800 --before default_reset --after hard_reset write_flash @flash_args
```

If your board uses another port, replace `/dev/ttyUSB0`.

## 5) Open serial monitor

If ESP-IDF monitor is available:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

Or with a generic terminal tool (for example, screen):

```bash
screen /dev/ttyUSB0 115200
```

## Troubleshooting

- Port busy:
  - Close any running monitor process and retry.
- Permission denied:
  - Add your user to `dialout` and relogin.
- No serial device found:
  - Reconnect USB cable and re-check device name.
