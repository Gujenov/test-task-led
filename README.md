# Test Task LED (ESP32, ESP-IDF)

## Status
- Step 0 completed: project bootstrap and base runtime configuration.
- Next step: implement `LED Blink Task` (1 second period).

## Implemented in Step 0
- Shared constants for GPIO, queue, stack sizes, priorities, and periods.
- Global FreeRTOS queue handle for counter-to-logger communication.
- Startup logs with basic runtime configuration visibility.
- Queue creation with error handling.

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
