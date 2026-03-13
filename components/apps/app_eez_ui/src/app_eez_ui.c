#include "app_eez_ui.h"

#include "esp_check.h"
#include "system_display.h"
#include "ui.h"

static const char *TAG = "app_eez_ui";

esp_err_t app_eez_ui_start(void)
{
    ESP_RETURN_ON_ERROR(system_display_register_ui_init_cb(ui_init), TAG, "register ui_init_cb failed");
    return system_ui_init();
}



