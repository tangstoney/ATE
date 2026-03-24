#include "app_update_runtime.h"

#include <stdlib.h>

#include "esp_check.h"

struct app_update_runtime {
    app_update_runtime_config_t config;
    app_update_runtime_snapshot_t snapshot;
};

static const char *TAG = "app_update_runtime";

static esp_err_t app_update_runtime_emit(app_update_runtime_handle_t handle,
                                         app_update_runtime_event_id_t event_id)
{
    app_update_runtime_event_t event = {
        .event_id = event_id,
        .snapshot = handle->snapshot,
    };

    if (handle->config.on_snapshot) {
        ESP_RETURN_ON_ERROR(handle->config.on_snapshot(handle->config.user_context, &handle->snapshot),
                            TAG,
                            "on_snapshot failed");
    }

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event), TAG, "on_event failed");
    }

    return ESP_OK;
}

esp_err_t app_update_runtime_init(const app_update_runtime_config_t *config,
                                  app_update_runtime_handle_t *out_handle)
{
    app_update_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc update runtime failed");

    if (config) {
        handle->config = *config;
    }

    handle->snapshot.active_source = APP_UPDATE_RUNTIME_SOURCE_NONE;
    handle->snapshot.last_result = ESP_OK;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_update_runtime_deinit(app_update_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_update_runtime_on_usb_ota_event(app_update_runtime_handle_t handle,
                                              const app_update_usb_ota_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->snapshot.active_source = APP_UPDATE_RUNTIME_SOURCE_USB_OTA;
    handle->snapshot.progress_percent = event->progress_percent;
    handle->snapshot.last_result = event->result;
    return app_update_runtime_emit(handle,
                                   event->event_id == APP_UPDATE_USB_OTA_EVENT_SUCCEEDED ||
                                           event->event_id == APP_UPDATE_USB_OTA_EVENT_FAILED
                                       ? APP_UPDATE_RUNTIME_EVENT_RESULT_UPDATED
                                       : APP_UPDATE_RUNTIME_EVENT_PROGRESS_UPDATED);
}

esp_err_t app_update_runtime_on_network_ota_event(app_update_runtime_handle_t handle,
                                                  const app_update_network_ota_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->snapshot.active_source = APP_UPDATE_RUNTIME_SOURCE_NETWORK_OTA;
    handle->snapshot.progress_percent = event->progress_percent;
    handle->snapshot.last_result = event->result;
    return app_update_runtime_emit(handle,
                                   event->event_id == APP_UPDATE_NETWORK_OTA_EVENT_SUCCEEDED ||
                                           event->event_id == APP_UPDATE_NETWORK_OTA_EVENT_FAILED
                                       ? APP_UPDATE_RUNTIME_EVENT_RESULT_UPDATED
                                       : APP_UPDATE_RUNTIME_EVENT_PROGRESS_UPDATED);
}

esp_err_t app_update_runtime_get_snapshot(app_update_runtime_handle_t handle,
                                          app_update_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}
