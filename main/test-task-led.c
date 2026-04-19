#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

/*
 * Step 0 bootstrap:
 * - Shared config/constants for future tasks
 * - Common queue handle
 * - Basic startup checks and logging
 */

#define LED_GPIO                    GPIO_NUM_2
#define COUNTER_QUEUE_LENGTH        8
#define COUNTER_QUEUE_ITEM_SIZE     sizeof(uint32_t)

#define TASK_STACK_SIZE_DEFAULT     2048
#define TASK_PRIORITY_BLINK         2
#define TASK_PRIORITY_COUNTER       3
#define TASK_PRIORITY_LOGGER        4

#define BLINK_PERIOD_MS             1000
#define COUNTER_PERIOD_MS           5000

static const char *TAG = "test-task-led";
static QueueHandle_t s_counter_queue = NULL;

void app_main(void)
{
	ESP_LOGI(TAG, "Bootstrapping project (Step 0)");
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
	ESP_LOGI(TAG, "Step 0 complete: ready to add Blink/Counter/Logger tasks");

	(void)TASK_STACK_SIZE_DEFAULT;
	(void)TASK_PRIORITY_BLINK;
	(void)TASK_PRIORITY_COUNTER;
	(void)TASK_PRIORITY_LOGGER;
	(void)BLINK_PERIOD_MS;
}
