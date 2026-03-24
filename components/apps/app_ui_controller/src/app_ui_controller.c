#include "app_ui_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

struct app_ui_controller {
    app_ui_controller_config_t config;
    bool started;
    app_ui_view_page_t current_page;
    app_ui_controller_test_config_t last_test_config;
};

static const char *TAG = "app_ui_controller";

static esp_err_t app_ui_controller_require_handle(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    return ESP_OK;
}

static esp_err_t app_ui_controller_require_started(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(handle->started, ESP_ERR_INVALID_STATE, TAG, "controller not started");
    return ESP_OK;
}

static esp_err_t app_ui_controller_emit(app_ui_controller_handle_t handle,
                                        const app_ui_controller_command_t *command)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_started(handle), TAG, "controller not ready");
    ESP_RETURN_ON_FALSE(command, ESP_ERR_INVALID_ARG, TAG, "command is NULL");

    if (handle->config.on_command) {
        return handle->config.on_command(handle->config.user_context, command);
    }

    return ESP_OK;
}

esp_err_t app_ui_controller_init(const app_ui_controller_config_t *config,
                                 app_ui_controller_handle_t *out_handle)
{
    app_ui_controller_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc controller failed");

    if (config) {
        handle->config = *config;
    }

    handle->current_page = APP_UI_VIEW_PAGE_MAIN;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_ui_controller_start(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");
    handle->started = true;
    return ESP_OK;
}

esp_err_t app_ui_controller_stop(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");
    handle->started = false;
    return ESP_OK;
}

esp_err_t app_ui_controller_deinit(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");
    free(handle);
    return ESP_OK;
}

esp_err_t app_ui_controller_on_start_test_request(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_START_TEST,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_stop_test_request(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_STOP_TEST,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_manual_rescan_instrument(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_INSTRUMENT,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_manual_rescan_module(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_MODULE,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_clear_instrument_binding(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_CLEAR_INSTRUMENT_BINDING,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_trigger_vision_check(app_ui_controller_handle_t handle)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_TRIGGER_VISION_CHECK,
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_start_usb_ota(app_ui_controller_handle_t handle, const char *update_file_path)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_START_USB_OTA,
    };

    if (update_file_path) {
        snprintf(command.payload.start_usb_ota.update_file_path,
                 sizeof(command.payload.start_usb_ota.update_file_path),
                 "%s",
                 update_file_path);
    }

    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_enable_usb_msc(app_ui_controller_handle_t handle, bool enable)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_ENABLE_USB_MSC,
        .payload.enable_usb_msc = {
            .enable = enable,
        },
    };
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_page_changed(app_ui_controller_handle_t handle, app_ui_view_page_t page)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_PAGE_CHANGED,
        .payload.page_changed = {
            .page = page,
        },
    };

    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");
    handle->current_page = page;
    return app_ui_controller_emit(handle, &command);
}

esp_err_t app_ui_controller_on_test_config_changed(app_ui_controller_handle_t handle,
                                                    const app_ui_controller_test_config_t *config)
{
    app_ui_controller_command_t command = {
        .command_id = APP_UI_CONTROLLER_COMMAND_TEST_CONFIG_CHANGED,
    };

    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_ERROR(app_ui_controller_require_handle(handle), TAG, "invalid handle");

    handle->last_test_config = *config;
    command.payload.test_config_changed.config = *config;
    return app_ui_controller_emit(handle, &command);
}
