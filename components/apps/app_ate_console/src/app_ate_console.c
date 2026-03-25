#include "app_ate_console.h"

#include "esp_check.h"

#include "app_ate_gateway.h"
#include "app_ui_controller.h"
#include "app_update_usb_ota.h"

static const char *TAG = "app_ate_console";

typedef struct {
    bool started;
    app_update_usb_ota_handle_t update_usb_ota;
    app_ate_gateway_handle_t ate_gateway;
    app_ui_controller_handle_t ui_controller;
} app_ate_console_context_t;

static app_ate_console_context_t s_console_ctx;

esp_err_t app_ate_console_start(void)
{
    if (s_console_ctx.started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(app_update_usb_ota_init(&s_console_ctx.update_usb_ota),
                        TAG,
                        "update usb ota init failed");

    ESP_RETURN_ON_ERROR(app_ate_gateway_init(&s_console_ctx.ate_gateway),
                        TAG,
                        "ate gateway init failed");

    ESP_RETURN_ON_ERROR(app_ui_controller_init(&s_console_ctx.ui_controller),
                        TAG,
                        "ui controller init failed");

    s_console_ctx.started = true;
    return ESP_OK;
}
