#include "system_instrument_service.h"

#include <stdbool.h>

#include "esp_check.h"
#include "system_comm_mgr.h"

static const char *TAG = "system_instrument_service";
static system_instrument_state_t s_state = SYSTEM_INSTRUMENT_STATE_UNINITIALIZED;
static bool s_initialized;

esp_err_t system_instrument_service_init(const system_instrument_service_config_t *config)
{
    system_uart_link_handle_t link = NULL;

    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_comm_mgr_get_uart_link(config->link_id, &link),
                        TAG, "system_comm_mgr_get_uart_link failed");

    s_initialized = true;
    s_state = SYSTEM_INSTRUMENT_STATE_IDLE;
    return ESP_OK;
}

esp_err_t system_instrument_scan(void)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    s_state = SYSTEM_INSTRUMENT_STATE_NOT_SUPPORTED;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t system_instrument_attach(uint32_t port_id)
{
    (void)port_id;
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    s_state = SYSTEM_INSTRUMENT_STATE_NOT_SUPPORTED;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t system_instrument_send(uint8_t dev_id,
                                 uint8_t cmd,
                                 const uint8_t *payload,
                                 size_t len)
{
    (void)dev_id;
    (void)cmd;
    (void)payload;
    (void)len;
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    s_state = SYSTEM_INSTRUMENT_STATE_NOT_SUPPORTED;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t system_instrument_get_state(system_instrument_state_t *out_state)
{
    ESP_RETURN_ON_FALSE(out_state, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    *out_state = s_state;
    return ESP_OK;
}
