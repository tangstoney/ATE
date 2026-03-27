#include "app_ate_console.h"

#include "app_ate_gateway.h"
#include "app_module_runtime.h"
#include "app_ui_controller.h"
#include "app_update_usb_ota.h"

typedef struct {
    bool started;
    app_module_runtime_handle_t module_runtime;
    app_update_usb_ota_handle_t update_usb_ota;
    app_ate_gateway_handle_t ate_gateway;
    app_ui_controller_handle_t ui_controller;
} app_ate_console_context_t;

static app_ate_console_context_t s_console_ctx;

esp_err_t app_ate_console_start(void)
{
    esp_err_t err = ESP_OK;

    if (s_console_ctx.started) {
        return ESP_OK;
    }

    err = app_module_runtime_init(&s_console_ctx.module_runtime);
    if (err != ESP_OK) {
        goto err;
    }

    err = app_update_usb_ota_init(&s_console_ctx.update_usb_ota);
    if (err != ESP_OK) {
        goto err;
    }

    err = app_ate_gateway_init(&s_console_ctx.ate_gateway);
    if (err != ESP_OK) {
        goto err;
    }

    err = app_ui_controller_init(&s_console_ctx.ui_controller);
    if (err != ESP_OK) {
        goto err;
    }

    // Protocol smoke tests stay in app_module_runtime debug helpers and are not
    // executed during the normal startup path.
    s_console_ctx.started = true;
    return ESP_OK;

err:
    if (s_console_ctx.ui_controller) {
        (void)app_ui_controller_deinit(s_console_ctx.ui_controller);
        s_console_ctx.ui_controller = NULL;
    }
    if (s_console_ctx.ate_gateway) {
        (void)app_ate_gateway_deinit(s_console_ctx.ate_gateway);
        s_console_ctx.ate_gateway = NULL;
    }
    if (s_console_ctx.update_usb_ota) {
        (void)app_update_usb_ota_deinit(s_console_ctx.update_usb_ota);
        s_console_ctx.update_usb_ota = NULL;
    }
    if (s_console_ctx.module_runtime) {
        (void)app_module_runtime_deinit(s_console_ctx.module_runtime);
        s_console_ctx.module_runtime = NULL;
    }
    return err;
}
