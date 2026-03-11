#include "system_log.h"

#include "esp_log.h"

static const char *TAG = "system_log";

void system_log_init(void)
{
    ESP_LOGI(TAG, "system log ready");
}
