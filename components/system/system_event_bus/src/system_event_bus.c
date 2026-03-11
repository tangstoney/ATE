#include "system_event_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_check.h"

static QueueHandle_t s_queue;
static const char *TAG = "system_event_bus";

esp_err_t system_event_bus_init(void)
{
    if (s_queue) {
        return ESP_OK;
    }
    s_queue = xQueueCreate(16, sizeof(system_event_t));
    ESP_RETURN_ON_FALSE(s_queue, ESP_ERR_NO_MEM, TAG, "xQueueCreate failed");
    return ESP_OK;
}

esp_err_t system_event_bus_post(const system_event_t *event)
{
    ESP_RETURN_ON_FALSE(s_queue && event, ESP_ERR_INVALID_STATE, TAG, "event bus not ready");
    return xQueueSend(s_queue, event, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t system_event_bus_get(system_event_t *event, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(s_queue && event, ESP_ERR_INVALID_STATE, TAG, "event bus not ready");
    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive(s_queue, event, ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}
