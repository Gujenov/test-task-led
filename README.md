# Test Task LED (ESP32, ESP-IDF)

## Status
- Step 0 completed: project bootstrap and base runtime configuration.
- Step 0.5 completed: firmware version auto-update script adapted for this project.
- Next step: implement `LED Blink Task` (1 second period).

## Implemented in Step 0
- Shared constants for GPIO, queue, stack sizes, priorities, and periods.
- Global FreeRTOS queue handle for counter-to-logger communication.
- Startup logs with basic runtime configuration visibility.
- Queue creation with error handling.

## Implemented in Step 0.5
- Firmware version is defined in `main/test-task-led.c` as `FIRMWARE_VERSION`.
- Version is printed at startup right after initialization begins.
- `update_version.py` updates this macro in-place using format:

```text
MCU.HW_VARIANT.RELEASE_TYPE.YYMMDD-XX
```

- Index rules:
  - if date tail (`YYMMDD`) matches current date, index increments (`00` -> `01` ... `99`),
  - if date tail differs from current date, index resets to `00`.

### Run version update

```bash
python3 update_version.py
```

## Build / flash (quick)
> Full setup instructions: see [ESP-IDF setup instructions.md](ESP-IDF%20setup%20instructions.md)

```bash
git clone <repo-url>
cd Test-task-led
idf.py set-target esp32
idf.py build
idf.py -p <PORT> flash monitor
```

## Notes
- LED GPIO currently set to `GPIO_NUM_2` (typical for ESP32 DevKit V1).
- If your board wiring differs, update `LED_GPIO` in [main/test-task-led.c](main/test-task-led.c).
