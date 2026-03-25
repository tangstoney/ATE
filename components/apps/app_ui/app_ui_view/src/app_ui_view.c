#include "app_ui_view.h"

#include "esp_check.h"
#include "system_display.h"
#include "ui.h"

static const char *TAG = "app_ui_view";

static bool s_started = false;

esp_err_t app_ui_view_init(void)
{
    if (s_started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(system_display_register_ui_init_cb(ui_init), TAG, "register ui_init failed");
    ESP_RETURN_ON_ERROR(system_ui_init(), TAG, "system ui init failed");
    s_started = true;
    return ESP_OK;
}

esp_err_t app_ui_view_deinit(void)
{
    s_started = false;
    return ESP_OK;
}
