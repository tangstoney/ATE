#include "app_ui_view.h"

#include "esp_check.h"
#include "system_display.h"
#include "ui.h"

static const char *TAG = "app_ui_view";

static bool s_initialized = false;
static bool s_started = false;
static uint8_t s_usb_ota_progress_percent = 0;

static esp_err_t app_ui_view_require_started(void)
{
    ESP_RETURN_ON_FALSE(s_started, ESP_ERR_INVALID_STATE, TAG, "ui view not started");
    return ESP_OK;
}

esp_err_t app_ui_view_init(void)
{
    s_initialized = true;
    return ESP_OK;
}

esp_err_t app_ui_view_start(void)
{
    if (s_started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(app_ui_view_init(), TAG, "ui view init failed");
    ESP_RETURN_ON_ERROR(system_display_register_ui_init_cb(ui_init), TAG, "register ui_init failed");
    ESP_RETURN_ON_ERROR(system_ui_init(), TAG, "system ui init failed");
    s_started = true;
    return ESP_OK;
}

esp_err_t app_ui_view_stop(void)
{
    s_started = false;
    return ESP_OK;
}

esp_err_t app_ui_view_deinit(void)
{
    s_started = false;
    s_initialized = false;
    return ESP_OK;
}

esp_err_t app_ui_view_show_default(void)
{
    return app_ui_view_show_page(APP_UI_VIEW_PAGE_MAIN);
}

esp_err_t app_ui_view_show_page(app_ui_view_page_t page)
{
    ESP_RETURN_ON_ERROR(app_ui_view_require_started(), TAG, "ui view not ready");

    switch (page) {
    case APP_UI_VIEW_PAGE_MAIN:
        loadScreen(SCREEN_ID_MAIN);
        break;
    case APP_UI_VIEW_PAGE_PAGE1:
        loadScreen(SCREEN_ID_PAGE1);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t app_ui_view_show_usb_ota_progress(uint8_t progress_percent)
{
    ESP_RETURN_ON_ERROR(app_ui_view_require_started(), TAG, "ui view not ready");
    s_usb_ota_progress_percent = progress_percent;
    return ESP_OK;
}
