#include "system_display.h"

#include "esp_check.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"

#include "driver_display.h"
#include "system_display_internal.h"

static const char *TAG = "system_display";

static driver_display_handle_t s_display = NULL;
static lv_display_t *s_disp = NULL;
static bool s_lvgl_started = false;
static system_display_ui_init_fn_t s_ui_init_cb = NULL;

esp_err_t system_display_init(void)
{
    if (s_disp) {
        return ESP_OK;
    }

    if (!s_display) {
        ESP_RETURN_ON_ERROR(driver_display_create(&s_display), TAG, "driver_display_create failed");
    }

    if (!s_lvgl_started) {
        esp_lcd_panel_handle_t panel = NULL;
        esp_lcd_panel_io_handle_t io = NULL;
        uint16_t w = 0;
        uint16_t h = 0;

        ESP_RETURN_ON_ERROR(driver_display_get_panel_handle(s_display, &panel, &io), TAG, "get panel failed");
        ESP_RETURN_ON_ERROR(driver_display_get_resolution(s_display, &w, &h), TAG, "get res failed");

        esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
        ESP_RETURN_ON_ERROR(esp_lv_adapter_init(&cfg), TAG, "esp_lv_adapter_init failed");

        esp_lv_adapter_display_config_t disp_cfg =
            ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
                panel,
                io,
                w,
                h,
                ESP_LV_ADAPTER_ROTATE_0);

        s_disp = esp_lv_adapter_register_display(&disp_cfg);
        ESP_RETURN_ON_FALSE(s_disp, ESP_FAIL, TAG, "register display failed");

        esp_lcd_touch_handle_t touch = NULL;
        if (driver_display_get_touch_handle(s_display, &touch) == ESP_OK && touch) {
            esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_disp, touch);
            lv_indev_t *indev = esp_lv_adapter_register_touch(&touch_cfg);
            ESP_RETURN_ON_FALSE(indev, ESP_FAIL, TAG, "register touch failed");
        }

        ESP_RETURN_ON_ERROR(esp_lv_adapter_start(), TAG, "esp_lv_adapter_start failed");
        s_lvgl_started = true;
    }

    return ESP_OK;
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
    return ESP_OK;
}

esp_err_t system_display_lock(void)
{
    return esp_lv_adapter_lock(-1);
}

esp_err_t system_display_unlock(void)
{
    esp_lv_adapter_unlock();
    return ESP_OK;
}

esp_err_t system_display_get_resolution(uint16_t *w, uint16_t *h)
{
    ESP_RETURN_ON_FALSE(w && h, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_get_resolution(s_display, w, h);
}

esp_err_t system_display_get_panel_handles(esp_lcd_panel_handle_t *out_panel,
                                           esp_lcd_panel_io_handle_t *out_io)
{
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_get_panel_handle(s_display, out_panel, out_io);
}
