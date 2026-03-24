#include "system_display.h"

#include <inttypes.h>
#include <stdio.h>

#include "esp_check.h"
#include "esp_system.h"

#include "driver_display.h"

static const char *TAG = "system_display";

static driver_display_handle_t s_display = NULL;
static system_display_ui_init_fn_t s_ui_init_cb = NULL;

esp_err_t system_display_init(void)
{
    if (!s_display) {
        ESP_RETURN_ON_ERROR(driver_display_create(&s_display), TAG, "driver_display_create failed");
    }

    return driver_display_start(s_display);
}

esp_err_t system_display_register_ui_init_cb(system_display_ui_init_fn_t cb)
{
    ESP_RETURN_ON_FALSE(cb, ESP_ERR_INVALID_ARG, TAG, "callback is NULL");
    s_ui_init_cb = cb;
    return ESP_OK;
}

esp_err_t system_ui_init(void)
{
    ESP_RETURN_ON_FALSE(s_ui_init_cb, ESP_ERR_INVALID_STATE, TAG, "ui init callback not registered");
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");
    ESP_RETURN_ON_ERROR(system_display_lock(), TAG, "system_display_lock failed");
    s_ui_init_cb();
    ESP_RETURN_ON_ERROR(system_display_unlock(), TAG, "system_display_unlock failed");


    // system_display_set_hw_pattern(MIPI_DSI_PATTERN_BAR_HORIZONTAL);
    printf("Minimum free heap size: %" PRIu32 " Mbytes\n", esp_get_minimum_free_heap_size()/(1024*1024));
    return ESP_OK;
}

esp_err_t system_display_lock(void)
{
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_lock(s_display, -1);
}

esp_err_t system_display_unlock(void)
{
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_unlock(s_display);
}

esp_err_t system_display_get_resolution(uint16_t *out_hor_res, uint16_t *out_ver_res)
{
    ESP_RETURN_ON_FALSE(out_hor_res && out_ver_res, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_get_resolution(s_display, out_hor_res, out_ver_res);
}

esp_err_t system_display_set_hw_pattern(mipi_dsi_pattern_type_t pattern)
{
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");
    return driver_display_set_hw_pattern(s_display, pattern);
}
