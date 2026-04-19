# Test Task LED (ESP32-S3, ESP-IDF)

## Status
- Step 0 completed: project bootstrap and base runtime configuration.
- Step 0.5 completed: firmware version auto-update script adapted for this project.
- Step 1 completed: `blink_task` toggles LED every 1 second.
- Step 2 completed: `counter_task` increments counter every 5 seconds and sends message to queue with timestamp.
- Next step: implement `logger_task` (receive queue messages, log interval and value).

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

- Version update script is executed automatically before each build from `CMakeLists.txt`.

### Run version update

```bash
python3 update_version.py
```

## Implemented in Step 1
- `blink_task` created as an independent FreeRTOS task.
- LED toggles every `BLINK_PERIOD_MS` using `vTaskDelay`.
- Task stack size is set to `3072` bytes for ESP32-S3 stability.

## Implemented in Step 2
- `counter_task` created with a 5-second period (`COUNTER_PERIOD_MS`).
- Uses `vTaskDelayUntil` for stable periodic scheduling.
- Captures send timestamp with `esp_timer_get_time()` right before `xQueueSend`.
- Sends `counter_message_t { value, sent_us }` into queue for the future logger task.
- If queue is full, counter message is dropped and warning is logged.

## Build / flash (quick)
> Full setup instructions: see [ESP-IDF setup instructions.md](ESP-IDF%20setup%20instructions.md)

```bash
git clone <repo-url>
cd Test-task-led
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

## Notes
- LED GPIO currently set to `GPIO_NUM_2`.
- If your board wiring differs, update `LED_GPIO` in [main/test-task-led.c](main/test-task-led.c).
- At Step 2, counter values are not printed continuously yet; they are queued.
- Continuous counter/interval logs will appear after `logger_task` is added in Step 3.
