#include "system_ui.h"

#include "esp_check.h"

#include "driver_display.h"

static const char *TAG = "system_ui";
static driver_display_handle_t s_display;

esp_err_t system_ui_service_init(void)
{
    if (s_display) {
        return ESP_OK;
    }
    return driver_display_create(&s_display);
}

esp_err_t system_ui_start_demo(void)
{
    ESP_RETURN_ON_ERROR(system_ui_service_init(), TAG, "system_ui_service_init failed");
    return ESP_OK;
}
