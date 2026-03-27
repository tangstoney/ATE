#include "system_module_bus.h"

#include "esp_check.h"

static const char *TAG = "system_module_bus";

static bool s_initialized;
static uint32_t s_selected_channel;

esp_err_t system_module_bus_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    s_selected_channel = 0;
    s_initialized = true;
    return ESP_OK;
}

esp_err_t system_module_bus_deinit(void)
{
    s_selected_channel = 0;
    s_initialized = false;
    return ESP_OK;
}

esp_err_t system_module_bus_select_channel(uint32_t channel_id)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "system_module_bus not initialized");

    /* TODO: bind channel selection to the real hardware bus selector or mux. */
    s_selected_channel = channel_id;
    return ESP_OK;
}

esp_err_t system_module_bus_get_selected_channel(uint32_t *out_channel_id)
{
    ESP_RETURN_ON_FALSE(out_channel_id, ESP_ERR_INVALID_ARG, TAG, "out_channel_id is NULL");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "system_module_bus not initialized");

    *out_channel_id = s_selected_channel;
    return ESP_OK;
}

esp_err_t system_module_bus_is_initialized(bool *out_initialized)
{
    ESP_RETURN_ON_FALSE(out_initialized, ESP_ERR_INVALID_ARG, TAG, "out_initialized is NULL");

    *out_initialized = s_initialized;
    return ESP_OK;
}
