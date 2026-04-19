#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

/*
 * Project-wide config and shared resources.
 *
 * Timing strategy:
 *   - LED Blink Task uses vTaskDelay — simple relative delay, sufficient for
 *     human-visible 1-second blinking where millisecond-level jitter is irrelevant.
 *   - Counter / Logger tasks (Steps 2-3) will use esp_timer_get_time() for
 *     high-resolution interval measurement (microsecond precision).
 */

#define LED_GPIO                    GPIO_NUM_2
#define COUNTER_QUEUE_LENGTH        8
#define COUNTER_QUEUE_ITEM_SIZE     sizeof(uint32_t)

#define TASK_STACK_SIZE_BLINK        2048
#define TASK_STACK_SIZE_DEFAULT     2048

#define TASK_PRIORITY_BLINK         2
#define TASK_PRIORITY_COUNTER       3
#define TASK_PRIORITY_LOGGER        4

#define BLINK_PERIOD_MS             1000
#define COUNTER_PERIOD_MS           5000

#define FIRMWARE_VERSION            "1.A0.3.260419-00"

static const char *TAG = "test-task-led";
static QueueHandle_t s_counter_queue = NULL;

/*
 * blink_task — Step 1
 *
 * Toggles the onboard LED once per BLINK_PERIOD_MS using vTaskDelay.
 * vTaskDelay is used intentionally here: blinking is a visual indicator
 * only, so simple relative delay is sufficient. Exact period accuracy is
 * not required and the simplicity keeps the task easy to read.
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

	bool led_on = false;

	while (true) {
		led_on = !led_on;
		gpio_set_level(LED_GPIO, led_on ? 1 : 0);
		vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Bootstrapping project (Step 0)");
	ESP_LOGI(TAG, "Firmware version: %s", FIRMWARE_VERSION);
	ESP_LOGI(TAG,
			 "Config: LED GPIO=%" PRIi32 ", queue_len=%" PRIi32 ", counter_period=%" PRIi32 " ms",
			 (int32_t)LED_GPIO,
			 (int32_t)COUNTER_QUEUE_LENGTH,
			 (int32_t)COUNTER_PERIOD_MS);

	s_counter_queue = xQueueCreate(COUNTER_QUEUE_LENGTH, COUNTER_QUEUE_ITEM_SIZE);
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
		NULL);

	if (blink_ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create blink_task");
		vQueueDelete(s_counter_queue);
		s_counter_queue = NULL;
		return;
	}

	ESP_LOGI(TAG, "Step 1 complete: blink_task running on GPIO %d", (int)LED_GPIO);

	(void)TASK_STACK_SIZE_DEFAULT;
	(void)TASK_PRIORITY_COUNTER;
	(void)TASK_PRIORITY_LOGGER;
	(void)COUNTER_PERIOD_MS;
}
