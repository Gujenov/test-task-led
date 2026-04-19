#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

/*
 * Project-wide config and shared resources.
 *
 * Timing strategy:
 *   - LED Blink Task uses vTaskDelay — simple relative delay, sufficient for
 *     human-visible 1-second blinking where millisecond-level jitter is irrelevant.
 *   - Counter / Logger tasks (Steps 2-3) will use esp_timer_get_time() for
 *     high-resolution interval measurement (microsecond precision).
 *
 * Stack sizes:
 *   Project is targeted for ESP32 (DevKit V1).
 *   3072-byte task stacks are intentionally kept as a conservative value to keep
 *   enough headroom for logging and future code growth.
 */

#define LED_GPIO                    GPIO_NUM_2
#define COUNTER_QUEUE_LENGTH        8

#define TASK_STACK_SIZE_BLINK       3072
#define TASK_STACK_SIZE_COUNTER     3072
#define TASK_STACK_SIZE_LOGGER      3072

#define TASK_PRIORITY_BLINK         2
#define TASK_PRIORITY_COUNTER       3
#define TASK_PRIORITY_LOGGER        4

#define BLINK_CYCLE_MS              1000
#define BLINK_ON_TIME_MS            150
#define COUNTER_PERIOD_MS           5000
#define LOGGER_DIAG_EVERY_MESSAGES  12

/*
 * Firmware version format: MCU.HW_VARIANT.RELEASE_TYPE.YYMMDD-XX
 * MCU index mapping used in this project:
 *   0 -> ESP32
 *   1 -> ESP32-S3
 */
#define FIRMWARE_VERSION            "0.A0.3.260420-13"

/*
 * Message sent from counter_task to logger_task via the queue.
 * sent_us is captured with esp_timer_get_time() (microsecond resolution)
 * immediately before xQueueSend so the logger can measure the exact
 * interval between consecutive queue sends.
 */
typedef struct {
	uint32_t value;     /* current counter value */
	int64_t  sent_us;   /* esp_timer timestamp at send time, microseconds */
} counter_message_t;

static const char *TAG = "test-task-led";
static QueueHandle_t s_counter_queue = NULL;
static TaskHandle_t s_blink_task_handle = NULL;
static TaskHandle_t s_counter_task_handle = NULL;
static TaskHandle_t s_logger_task_handle = NULL;

/*
 * Cleanup helper for partial startup failures.
 * Deletes already created tasks and queue so app_main can fail atomically.
 */
static void cleanup_startup_resources(void)
{
	if (s_logger_task_handle != NULL) {
		vTaskDelete(s_logger_task_handle);
		s_logger_task_handle = NULL;
	}

	if (s_counter_task_handle != NULL) {
		vTaskDelete(s_counter_task_handle);
		s_counter_task_handle = NULL;
	}

	if (s_blink_task_handle != NULL) {
		vTaskDelete(s_blink_task_handle);
		s_blink_task_handle = NULL;
	}

	if (s_counter_queue != NULL) {
		vQueueDelete(s_counter_queue);
		s_counter_queue = NULL;
	}
}

/*
 * Runtime diagnostics to help catch queue pressure and stack risks early.
 * Note: uxTaskGetStackHighWaterMark returns words, not bytes.
 */
static void log_runtime_diagnostics(uint32_t processed_messages)
{
	UBaseType_t queued = uxQueueMessagesWaiting(s_counter_queue);
	UBaseType_t blink_hwm_words = (s_blink_task_handle != NULL) ? uxTaskGetStackHighWaterMark(s_blink_task_handle) : 0;
	UBaseType_t counter_hwm_words = (s_counter_task_handle != NULL) ? uxTaskGetStackHighWaterMark(s_counter_task_handle) : 0;
	UBaseType_t logger_hwm_words = (s_logger_task_handle != NULL) ? uxTaskGetStackHighWaterMark(s_logger_task_handle) : 0;

	ESP_LOGI(TAG,
			 "diag: processed=%" PRIu32 ", queue_waiting=%" PRIu32 ", stack_hwm_words{blink=%" PRIu32 ", counter=%" PRIu32 ", logger=%" PRIu32 "}",
			 processed_messages,
			 (uint32_t)queued,
			 (uint32_t)blink_hwm_words,
			 (uint32_t)counter_hwm_words,
			 (uint32_t)logger_hwm_words);
}

/*
 * logger_task — Step 3
 *
 * Receives counter messages from queue and prints the measured interval
 * between consecutive sends using sent_us captured in counter_task.
 * If queue overload drops messages, logger will report a counter jump.
 */
static void logger_task(void *arg)
{
	(void)arg;

	counter_message_t msg;
	bool has_prev = false;
	int64_t prev_sent_us = 0;
	uint32_t expected_counter = 1;
	uint32_t processed = 0;

	while (true) {
		BaseType_t received = xQueueReceive(s_counter_queue, &msg, portMAX_DELAY);
		if (received != pdPASS) {
			continue;
		}

		if (msg.value != expected_counter) {
			ESP_LOGW(TAG,
					 "logger_task: counter jump detected, expected=%" PRIu32 ", got=%" PRIu32,
					 expected_counter,
					 msg.value);
			expected_counter = msg.value;
		}

		if (!has_prev) {
			ESP_LOGI(TAG,
					 "logger_task: counter=%" PRIu32 ", first sample, sent_us=%" PRIi64,
					 msg.value,
					 msg.sent_us);
			has_prev = true;
		} else {
			int64_t delta_us = msg.sent_us - prev_sent_us;
			if (delta_us <= 0) {
				ESP_LOGW(TAG,
						 "logger_task: non-monotonic timestamp, prev=%" PRIi64 ", curr=%" PRIi64,
						 prev_sent_us,
						 msg.sent_us);
			}
			double delta_ms = (double)delta_us / 1000.0;
			ESP_LOGI(TAG,
					 "logger_task: counter=%" PRIu32 ", delta=%" PRIi64 " us (%.3f ms), sent_us=%" PRIi64,
					 msg.value,
					 delta_us,
					 delta_ms,
					 msg.sent_us);
		}

		prev_sent_us = msg.sent_us;
		expected_counter = msg.value + 1;
		processed++;

		if ((processed % LOGGER_DIAG_EVERY_MESSAGES) == 0) {
			log_runtime_diagnostics(processed);
		}
	}
}

/*
 * counter_task — Step 2
 *
 * Increments an internal counter every COUNTER_PERIOD_MS and sends the value
 * together with the current esp_timer timestamp to the queue.
 *
 * vTaskDelayUntil is used instead of vTaskDelay so that each period starts
 * from the previous wake-up tick, absorbing any execution time inside the
 * loop. This keeps the send interval accurate regardless of small scheduling
 * jitter. esp_timer_get_time() is captured right before xQueueSend to give
 * the logger the most precise possible send timestamp.
 *
 * If the queue is full the message is dropped and a warning is logged.
 * The counter is NOT rolled back — the value sequence stays monotonic so
 * the logger can detect a gap if a message is ever dropped.
 */
static void counter_task(void *arg)
{
	(void)arg;

	uint32_t counter = 0;
	TickType_t last_wake = xTaskGetTickCount();
	const TickType_t period_ticks = pdMS_TO_TICKS(COUNTER_PERIOD_MS);

	while (true) {
		vTaskDelayUntil(&last_wake, period_ticks);

		counter_message_t msg = {
			.value   = ++counter,
			.sent_us = esp_timer_get_time(),
		};

		BaseType_t sent = xQueueSend(s_counter_queue, &msg, 0);
		if (sent != pdPASS) {
			ESP_LOGW(TAG, "counter_task: queue full, dropping counter=%" PRIu32, msg.value);
		}
	}
}

/*
 * blink_task — Step 1
 *
 * Produces one visible blink per second: short ON pulse + OFF remainder.
 * This interpretation matches "blink once per second" as a pulse pattern,
 * not a 50% duty-cycle square wave.
 *
 * GPIO is configured inside the task so the task is self-contained and
 * can be created / destroyed independently.
 */
static void blink_task(void *arg)
{
	(void)arg;

	gpio_config_t io_conf = {
		.pin_bit_mask  = (1ULL << LED_GPIO),
		.mode          = GPIO_MODE_OUTPUT,
		.pull_up_en    = GPIO_PULLUP_DISABLE,
		.pull_down_en  = GPIO_PULLDOWN_DISABLE,
		.intr_type     = GPIO_INTR_DISABLE,
	};

	esp_err_t ret = gpio_config(&io_conf);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "blink_task: GPIO config failed: %s", esp_err_to_name(ret));
		vTaskDelete(NULL);
		return;
	}

	const TickType_t on_ticks = pdMS_TO_TICKS(BLINK_ON_TIME_MS);
	const TickType_t off_ticks = pdMS_TO_TICKS(BLINK_CYCLE_MS - BLINK_ON_TIME_MS);

	while (true) {
		gpio_set_level(LED_GPIO, 1);
		vTaskDelay(on_ticks);
		gpio_set_level(LED_GPIO, 0);
		vTaskDelay(off_ticks);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Firmware version: %s", FIRMWARE_VERSION);
	ESP_LOGI(TAG,
			 "Config: LED GPIO=%" PRIi32 ", queue_len=%" PRIi32 ", counter_period=%" PRIi32 " ms",
			 (int32_t)LED_GPIO,
			 (int32_t)COUNTER_QUEUE_LENGTH,
			 (int32_t)COUNTER_PERIOD_MS);

	s_counter_queue = xQueueCreate(COUNTER_QUEUE_LENGTH, sizeof(counter_message_t));
	if (s_counter_queue == NULL) {
		ESP_LOGE(TAG, "Failed to create counter queue");
		return;
	}

	ESP_LOGI(TAG, "Counter queue created successfully");

	BaseType_t blink_ok = xTaskCreate(
		blink_task,
		"blink_task",
		TASK_STACK_SIZE_BLINK,
		NULL,
		TASK_PRIORITY_BLINK,
		&s_blink_task_handle);

	if (blink_ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create blink_task");
		cleanup_startup_resources();
		return;
	}

	ESP_LOGI(TAG, "Step 1 complete: blink_task running on GPIO %d", (int)LED_GPIO);

	BaseType_t counter_ok = xTaskCreate(
		counter_task,
		"counter_task",
		TASK_STACK_SIZE_COUNTER,
		NULL,
		TASK_PRIORITY_COUNTER,
		&s_counter_task_handle);

	if (counter_ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create counter_task");
		cleanup_startup_resources();
		return;
	}

	ESP_LOGI(TAG, "Step 2 complete: counter_task running, period=%d ms", COUNTER_PERIOD_MS);

	BaseType_t logger_ok = xTaskCreate(
		logger_task,
		"logger_task",
		TASK_STACK_SIZE_LOGGER,
		NULL,
		TASK_PRIORITY_LOGGER,
		&s_logger_task_handle);

	if (logger_ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create logger_task");
		cleanup_startup_resources();
		return;
	}

	ESP_LOGI(TAG, "Step 3 complete: logger_task running");
	ESP_LOGI(TAG, "Step 4 complete: runtime diagnostics and startup cleanup enabled");
}
