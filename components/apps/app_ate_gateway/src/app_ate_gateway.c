#include "app_ate_gateway.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_ate_gateway {
    app_ate_gateway_connection_state_t connection_state;
    bool server_online;
    bool device_online;
    uint32_t session_id;
};

static const char *TAG = "app_ate_gateway";

ESP_EVENT_DEFINE_BASE(APP_ATE_GATEWAY_EVENT);

static esp_err_t app_ate_gateway_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_ATE_GATEWAY_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_ate_gateway_publish_event(app_ate_gateway_handle_t handle,
                                               app_ate_gateway_event_id_t event_id,
                                               esp_err_t result,
                                               size_t payload_len)
{
    app_ate_gateway_event_t event = {
        .event_id = event_id,
        .connection_state = handle->connection_state,
        .session_id = handle->session_id,
        .server_online = handle->server_online,
        .device_online = handle->device_online,
        .result = result,
        .payload_len = payload_len,
    };
    return app_ate_gateway_post(APP_ATE_GATEWAY_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_ate_gateway_require_handle(app_ate_gateway_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    return ESP_OK;
}

static bool app_ate_gateway_is_ready(const app_ate_gateway_handle_t handle)
{
    return handle->connection_state == APP_ATE_GATEWAY_CONNECTION_STATE_READY &&
           handle->server_online &&
           handle->device_online;
}

esp_err_t app_ate_gateway_init(app_ate_gateway_handle_t *out_handle)
{
    app_ate_gateway_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc gateway failed");

    handle->connection_state = APP_ATE_GATEWAY_CONNECTION_STATE_DISCONNECTED;

    ret = app_ate_gateway_publish_event(handle, APP_ATE_GATEWAY_EVENT_STARTED, ESP_OK, 0);
    if (ret != ESP_OK) {
        free(handle);
        return ret;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_ate_gateway_deinit(app_ate_gateway_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ate_gateway_require_handle(handle), TAG, "invalid handle");
    free(handle);
    return ESP_OK;
}

esp_err_t app_ate_gateway_on_server_command(app_ate_gateway_handle_t handle,
                                            const app_ate_gateway_server_command_t *command)
{
    app_ate_gateway_device_command_t device_command = {0};

    ESP_RETURN_ON_ERROR(app_ate_gateway_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(command, ESP_ERR_INVALID_ARG, TAG, "command is NULL");
    ESP_RETURN_ON_FALSE(app_ate_gateway_is_ready(handle), ESP_ERR_INVALID_STATE, TAG, "gateway not ready");

    device_command.session_id = handle->session_id;
    device_command.command_id = command->command_id;
    device_command.len = command->len;
    if (device_command.len > sizeof(device_command.payload)) {
        device_command.len = sizeof(device_command.payload);
    }
    memcpy(device_command.payload, command->payload, device_command.len);

    ESP_RETURN_ON_ERROR(app_ate_gateway_post(APP_ATE_GATEWAY_BUS_EVENT_DEVICE_COMMAND,
                                             &device_command,
                                             sizeof(device_command)),
                        TAG,
                        "post device command failed");

    return app_ate_gateway_publish_event(handle,
                                         APP_ATE_GATEWAY_EVENT_SERVER_COMMAND_ROUTED,
                                         ESP_OK,
                                         device_command.len);
}

esp_err_t app_ate_gateway_on_device_data(app_ate_gateway_handle_t handle,
                                         const app_ate_gateway_device_data_t *device_data)
{
    app_ate_gateway_server_data_t server_data = {0};

    ESP_RETURN_ON_ERROR(app_ate_gateway_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(device_data, ESP_ERR_INVALID_ARG, TAG, "device_data is NULL");
    ESP_RETURN_ON_FALSE(app_ate_gateway_is_ready(handle), ESP_ERR_INVALID_STATE, TAG, "gateway not ready");

    server_data.session_id = handle->session_id;
    server_data.len = device_data->len;
    if (server_data.len > sizeof(server_data.payload)) {
        server_data.len = sizeof(server_data.payload);
    }
    memcpy(server_data.payload, device_data->payload, server_data.len);

    ESP_RETURN_ON_ERROR(app_ate_gateway_post(APP_ATE_GATEWAY_BUS_EVENT_SERVER_DATA,
                                             &server_data,
                                             sizeof(server_data)),
                        TAG,
                        "post server data failed");

    return app_ate_gateway_publish_event(handle,
                                         APP_ATE_GATEWAY_EVENT_DEVICE_DATA_FORWARDED,
                                         ESP_OK,
                                         server_data.len);
}

esp_err_t app_ate_gateway_on_connection_event(app_ate_gateway_handle_t handle,
                                              const app_ate_gateway_connection_event_t *event)
{
    const bool became_ready =
        (handle->connection_state != APP_ATE_GATEWAY_CONNECTION_STATE_READY) &&
        (event->state == APP_ATE_GATEWAY_CONNECTION_STATE_READY);

    ESP_RETURN_ON_ERROR(app_ate_gateway_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->connection_state = event->state;
    handle->server_online = event->server_online;
    handle->device_online = event->device_online;
    if (became_ready) {
        handle->session_id++;
    }

    return app_ate_gateway_publish_event(handle, APP_ATE_GATEWAY_EVENT_CONNECTION_CHANGED, event->result, 0);
}

esp_err_t app_ate_gateway_get_connection_state(app_ate_gateway_handle_t handle,
                                               app_ate_gateway_connection_state_t *out_state)
{
    ESP_RETURN_ON_ERROR(app_ate_gateway_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(out_state, ESP_ERR_INVALID_ARG, TAG, "out_state is NULL");

    *out_state = handle->connection_state;
    return ESP_OK;
}
