#include "system_init.h"

#include "esp_check.h"

#include "system_config.h"
#include "system_event_bus.h"
#include "system_log.h"
#include "system_network.h"
#include "system_storage.h"
#include "system_display.h"

static const char *TAG = "system_init";

esp_err_t system_init(void)
{
    system_log_init();
    ESP_RETURN_ON_ERROR(system_config_init(), TAG, "system_config_init failed");
    ESP_RETURN_ON_ERROR(system_storage_init(), TAG, "system_storage_init failed");
    ESP_RETURN_ON_ERROR(system_event_bus_init(), TAG, "system_event_bus_init failed");
    ESP_RETURN_ON_ERROR(system_network_init(), TAG, "system_network_init failed");
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");
    return ESP_OK;
}
