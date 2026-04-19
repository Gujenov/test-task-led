# Test Task LED

Firmware for ESP32 DevKit V1 (ESP-IDF, C, FreeRTOS).

The application runs three tasks:
- LED Blink Task: one blink per second (150 ms ON + 850 ms OFF).
- Counter Task: increments a counter every 5 seconds and pushes it to a queue.
- Logger Task: receives queue messages, logs the value, and logs measured send interval.

## Hardware target
- Current target: ESP32 (`idf.py set-target esp32`).

## What is implemented
- Queue-based communication between Counter and Logger tasks.
- Interval measurement using `esp_timer_get_time()` (microsecond resolution).
- Startup fault handling: if task creation fails, already-created resources are cleaned up.
- Runtime diagnostics every 12 messages:
  - queue occupancy,
  - stack high-water marks for all tasks.
- Pre-build firmware version auto-update (`update_version.py` is hooked from `CMakeLists.txt`).

## Firmware version format
Version format:

```text
MCU.HW_VARIANT.RELEASE_TYPE.YYMMDD-XX
```

MCU index mapping:
- `0` -> ESP32
- `1` -> ESP32-S3

Examples:
- `0.A0.3.260420-04` means ESP32 build.
- `1.A0.3.260420-04` means ESP32-S3 build.

## Quick start (already have ESP-IDF installed)
1. Clone project:

```bash
git clone https://github.com/Gujenov/test-task-led.git
cd test-task-led
```

2. Load ESP-IDF environment:

```bash
. $HOME/esp/esp-idf/export.sh
```

3. Set target and build:

```bash
idf.py set-target esp32
idf.py build
```

4. Flash and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

If this repo was previously built under ESP32-S3 on your machine, run `idf.py set-target esp32` again before building.

## About firmware files in this repository
- Prebuilt firmware binaries are **not** stored in git (`build/` is ignored).
- After `idf.py build`, generated files are created locally in `build/`, including:
  - `build/bootloader/bootloader.bin`
  - `build/partition_table/partition-table.bin`
  - `build/test-task-led.bin`
  - `build/flash_args` (used by ESP-IDF/esptool during flashing)
- So reviewers can flash the board by following this repo's instructions, but only after local build.
- To flash from generated local artifacts, use either:
  - `idf.py -p /dev/ttyUSB0 flash`
  - or from `build/`: `python -m esptool --chip esp32 write_flash @flash_args`

## Full setup from clean Linux machine
See [docs/ESP-IDF-Setup-and-GitHub.md](docs/ESP-IDF-Setup-and-GitHub.md).

It includes:
- ESP-IDF installation,
- shell environment setup,
- GitHub clone/push workflow,
- build/flash/monitor commands,
- common troubleshooting.

## Fallback: prebuilt firmware in repository
- A ready-to-flash ESP32 firmware bundle is included in `prebuilt/esp32/`.
- Use this only as a fallback when local build is not possible.
- Flashing instructions are in [docs/How-to-Flash-Prebuilt-Firmware.md](docs/How-to-Flash-Prebuilt-Firmware.md).

## Important notes
- Default LED pin in this project is `GPIO_NUM_2`.
- If your board uses another LED pin, change `LED_GPIO` in [main/test-task-led.c](main/test-task-led.c).
- Stack values are intentionally conservative for robustness and diagnostics.
- Blink semantics in this project: one short pulse per second (`BLINK_ON_TIME_MS=150`, `BLINK_CYCLE_MS=1000`).

## Acceptance checklist (latest local run)
- [x] Target set to `esp32`.
- [x] Build succeeds with ESP-IDF (`idf.py build`).
- [x] Flash succeeds (`idf.py -p /dev/ttyUSB0 flash`).
- [x] Monitor shows startup logs and all 3 tasks running.
- [x] Logger prints ~5000 ms interval between counter messages.
- [x] Periodic diagnostics line appears (`diag: processed=...`).
