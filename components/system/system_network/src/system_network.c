#include "system_network.h"

#include "esp_check.h"

#include "driver_network.h"

static driver_network_handle_t s_network;
static const char *TAG = "system_network";

esp_err_t system_network_init(void)
{
    if (s_network) {
        return ESP_OK;
    }
    return driver_network_create(&s_network);
}

esp_err_t system_network_start(void)
{
    ESP_RETURN_ON_ERROR(system_network_init(), TAG, "network init failed");
    return ESP_OK;
}
