#include "app_update_runtime.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_update_runtime {
    app_update_runtime_snapshot_t snapshot;
};

static const char *TAG = "app_update_runtime";

ESP_EVENT_DEFINE_BASE(APP_UPDATE_RUNTIME_EVENT);

static esp_err_t app_update_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_UPDATE_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_update_runtime_publish(app_update_runtime_handle_t handle,
                                            app_update_runtime_event_id_t event_id)
{
    app_update_runtime_event_t event = {
        .event_id = event_id,
        .snapshot = handle->snapshot,
    };
    ESP_RETURN_ON_ERROR(app_update_runtime_post(APP_UPDATE_RUNTIME_BUS_EVENT_SNAPSHOT,
                                                &handle->snapshot,
                                                sizeof(handle->snapshot)),
                        TAG,
                        "post snapshot failed");
    return app_update_runtime_post(APP_UPDATE_RUNTIME_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

esp_err_t app_update_runtime_init(app_update_runtime_handle_t *out_handle)
{
    app_update_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc update runtime failed");

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
    return app_update_runtime_publish(handle,
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
    return app_update_runtime_publish(handle,
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
