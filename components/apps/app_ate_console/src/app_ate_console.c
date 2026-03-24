#include "app_ate_console.h"

#include "esp_check.h"
#include "esp_log.h"

#include "app_ate_gateway.h"
#include "app_ui_controller.h"
#include "app_ui_state_binding.h"
#include "app_ui_view.h"
#include "app_update_usb_ota.h"

static const char *TAG = "app_ate_console";

typedef struct {
    bool started;
    app_ui_controller_handle_t ui_controller;
    app_ui_state_binding_handle_t ui_state_binding;
    app_update_usb_ota_handle_t update_usb_ota;
    app_ate_gateway_handle_t ate_gateway;
} app_ate_console_context_t;

static app_ate_console_context_t s_console_ctx;

static esp_err_t app_ate_console_on_gateway_event(void *user_context,
                                                  const app_ate_gateway_event_t *event)
{
    app_ate_console_context_t *ctx = user_context;
    app_ui_state_binding_network_state_event_t network_event = {
        .state = APP_UI_STATE_BINDING_NETWORK_STATE_UNKNOWN,
        .server_connected = false,
        .ip_ready = false,
    };

    ESP_RETURN_ON_FALSE(ctx && event, ESP_ERR_INVALID_ARG, TAG, "invalid gateway callback args");

    switch (event->connection_state) {
    case APP_ATE_GATEWAY_CONNECTION_STATE_CONNECTING:
        network_event.state = APP_UI_STATE_BINDING_NETWORK_STATE_CONNECTING;
        break;
    case APP_ATE_GATEWAY_CONNECTION_STATE_CONNECTED:
        network_event.state = APP_UI_STATE_BINDING_NETWORK_STATE_CONNECTING;
        network_event.ip_ready = true;
        break;
    case APP_ATE_GATEWAY_CONNECTION_STATE_READY:
        network_event.state = APP_UI_STATE_BINDING_NETWORK_STATE_READY;
        network_event.server_connected = true;
        network_event.ip_ready = true;
        break;
    case APP_ATE_GATEWAY_CONNECTION_STATE_FAULT:
        network_event.state = APP_UI_STATE_BINDING_NETWORK_STATE_FAULT;
        break;
    case APP_ATE_GATEWAY_CONNECTION_STATE_DISCONNECTED:
    default:
        network_event.state = APP_UI_STATE_BINDING_NETWORK_STATE_DISCONNECTED;
        break;
    }

    ESP_LOGI(TAG,
             "gateway event=%d state=%d result=%s",
             event->event_id,
             event->connection_state,
             esp_err_to_name(event->result));
    return app_ui_state_binding_on_network_state_event(ctx->ui_state_binding, &network_event);
}

static esp_err_t app_ate_console_on_gateway_device_command(
    void *user_context,
    const app_ate_gateway_device_command_t *device_command)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(device_command, ESP_ERR_INVALID_ARG, TAG, "device_command is NULL");
    ESP_LOGI(TAG,
             "gateway routed server command, session=%lu command=%u len=%u",
             (unsigned long)device_command->session_id,
             (unsigned)device_command->command_id,
             (unsigned)device_command->len);
    return ESP_OK;
}

static esp_err_t app_ate_console_on_gateway_server_data(void *user_context,
                                                        const app_ate_gateway_server_data_t *server_data)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(server_data, ESP_ERR_INVALID_ARG, TAG, "server_data is NULL");
    ESP_LOGI(TAG,
             "gateway forwarded device data to server, session=%lu len=%u",
             (unsigned long)server_data->session_id,
             (unsigned)server_data->len);
    return ESP_OK;
}

static esp_err_t app_ate_console_on_update_progress(void *user_context,
                                                    const app_update_usb_ota_progress_t *progress)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(progress, ESP_ERR_INVALID_ARG, TAG, "progress is NULL");
    return app_ui_view_show_usb_ota_progress(progress->progress_percent);
}

static esp_err_t app_ate_console_on_update_result(void *user_context,
                                                  const app_update_usb_ota_result_t *result)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(result, ESP_ERR_INVALID_ARG, TAG, "result is NULL");
    ESP_LOGI(TAG,
             "usb ota result=%s reboot_required=%d file=%s",
             esp_err_to_name(result->result),
             result->reboot_required,
             result->active_file_path);
    return ESP_OK;
}

static esp_err_t app_ate_console_on_update_event(void *user_context,
                                                 const app_update_usb_ota_event_t *event)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");
    ESP_LOGI(TAG,
             "usb ota event=%d status=%d progress=%u file=%s",
             event->event_id,
             event->status,
             event->progress_percent,
             event->active_file_path);
    return ESP_OK;
}

static esp_err_t app_ate_console_on_ui_state_update(
    void *user_context,
    const app_ui_state_binding_ui_state_update_t *update)
{
    (void)user_context;
    ESP_RETURN_ON_FALSE(update, ESP_ERR_INVALID_ARG, TAG, "update is NULL");
    ESP_LOGI(TAG,
             "ui state update seq=%lu system=%d network=%d test=%d",
             (unsigned long)update->sequence,
             update->status_snapshot.system_state,
             update->status_snapshot.network_state,
             update->status_snapshot.test_state);
    return ESP_OK;
}

static esp_err_t app_ate_console_handle_ui_command(void *user_context,
                                                   const app_ui_controller_command_t *command)
{
    app_ate_console_context_t *ctx = user_context;

    ESP_RETURN_ON_FALSE(ctx && command, ESP_ERR_INVALID_ARG, TAG, "invalid ui command args");

    switch (command->command_id) {
    case APP_UI_CONTROLLER_COMMAND_PAGE_CHANGED:
        return app_ui_view_show_page(command->payload.page_changed.page);
    case APP_UI_CONTROLLER_COMMAND_START_USB_OTA:
        if (command->payload.start_usb_ota.update_file_path[0] != '\0') {
            ESP_RETURN_ON_ERROR(app_update_usb_ota_on_file_detected(
                                    ctx->update_usb_ota,
                                    command->payload.start_usb_ota.update_file_path),
                                TAG,
                                "record ui-selected usb ota file failed");
        } else {
            ESP_RETURN_ON_ERROR(app_update_usb_ota_on_file_detected(ctx->update_usb_ota, "/usb/ota.bin"),
                                TAG,
                                "record default usb ota file failed");
        }
        ESP_RETURN_ON_ERROR(app_update_usb_ota_on_usb_insert(ctx->update_usb_ota, true),
                            TAG,
                            "mark usb media ready failed");
        return app_update_usb_ota_start(ctx->update_usb_ota);
    case APP_UI_CONTROLLER_COMMAND_START_TEST:
    case APP_UI_CONTROLLER_COMMAND_STOP_TEST:
    case APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_INSTRUMENT:
    case APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_MODULE:
    case APP_UI_CONTROLLER_COMMAND_CLEAR_INSTRUMENT_BINDING:
    case APP_UI_CONTROLLER_COMMAND_TRIGGER_VISION_CHECK:
    case APP_UI_CONTROLLER_COMMAND_ENABLE_USB_MSC:
    case APP_UI_CONTROLLER_COMMAND_TEST_CONFIG_CHANGED:
        ESP_LOGI(TAG, "ui command %d accepted, app runtime follow-up is TODO", command->command_id);
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t app_ate_console_start(void)
{
    app_ui_controller_config_t ui_controller_config = {
        .on_command = app_ate_console_handle_ui_command,
        .user_context = &s_console_ctx,
    };
    app_ui_state_binding_config_t ui_state_binding_config = {
        .on_ui_state_update = app_ate_console_on_ui_state_update,
        .user_context = &s_console_ctx,
        .module_stale_timeout_ms = 3000,
        .instrument_stale_timeout_ms = 3000,
        .network_stale_timeout_ms = 5000,
    };
    app_update_usb_ota_config_t update_usb_ota_config = {
        .default_ota_file_path = "/usb/ota.bin",
        .on_progress = app_ate_console_on_update_progress,
        .on_result = app_ate_console_on_update_result,
        .on_event = app_ate_console_on_update_event,
        .user_context = &s_console_ctx,
    };
    app_ate_gateway_config_t ate_gateway_config = {
        .on_device_command = app_ate_console_on_gateway_device_command,
        .on_server_data = app_ate_console_on_gateway_server_data,
        .on_event = app_ate_console_on_gateway_event,
        .user_context = &s_console_ctx,
    };
    app_ui_state_binding_system_state_event_t system_ready_event = {
        .state = APP_UI_STATE_BINDING_SYSTEM_STATE_READY,
        .maintenance_mode = false,
        .fault_active = false,
    };

    if (s_console_ctx.started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(app_ui_view_init(), TAG, "ui view init failed");
    ESP_RETURN_ON_ERROR(app_ui_view_start(), TAG, "ui view start failed");
    ESP_RETURN_ON_ERROR(app_ui_view_show_default(), TAG, "show default ui page failed");

    ESP_RETURN_ON_ERROR(app_ui_controller_init(&ui_controller_config, &s_console_ctx.ui_controller),
                        TAG,
                        "ui controller init failed");
    ESP_RETURN_ON_ERROR(app_ui_controller_start(s_console_ctx.ui_controller),
                        TAG,
                        "ui controller start failed");

    ESP_RETURN_ON_ERROR(app_ui_state_binding_init(&ui_state_binding_config, &s_console_ctx.ui_state_binding),
                        TAG,
                        "ui state binding init failed");
    ESP_RETURN_ON_ERROR(app_ui_state_binding_start(s_console_ctx.ui_state_binding),
                        TAG,
                        "ui state binding start failed");
    ESP_RETURN_ON_ERROR(app_ui_state_binding_on_system_state_event(s_console_ctx.ui_state_binding,
                                                                   &system_ready_event),
                        TAG,
                        "publish initial system state failed");

    ESP_RETURN_ON_ERROR(app_update_usb_ota_init(&update_usb_ota_config, &s_console_ctx.update_usb_ota),
                        TAG,
                        "update usb ota init failed");

    ESP_RETURN_ON_ERROR(app_ate_gateway_init(&ate_gateway_config, &s_console_ctx.ate_gateway),
                        TAG,
                        "ate gateway init failed");
    ESP_RETURN_ON_ERROR(app_ate_gateway_start(s_console_ctx.ate_gateway), TAG, "ate gateway start failed");

    s_console_ctx.started = true;
    return ESP_OK;
}
