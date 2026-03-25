#include "app_update_network_ota.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_update_network_ota {
    app_update_network_ota_progress_t progress;
    app_update_network_ota_result_t result;
};

static const char *TAG = "app_upd_net_ota";

ESP_EVENT_DEFINE_BASE(APP_UPDATE_NETWORK_OTA_EVENT);

static esp_err_t app_update_network_ota_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_UPDATE_NETWORK_OTA_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_update_network_ota_publish_event(app_update_network_ota_handle_t handle,
                                                      app_update_network_ota_event_id_t event_id)
{
    app_update_network_ota_event_t event = {
        .event_id = event_id,
        .status = handle->progress.status,
        .progress_percent = handle->progress.progress_percent,
        .result = handle->result.result,
    };
    return app_update_network_ota_post(APP_UPDATE_NETWORK_OTA_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_update_network_ota_publish_progress(app_update_network_ota_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_update_network_ota_post(APP_UPDATE_NETWORK_OTA_BUS_EVENT_PROGRESS,
                                                    &handle->progress,
                                                    sizeof(handle->progress)),
                        TAG,
                        "post progress failed");
    return app_update_network_ota_publish_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_PROGRESS_UPDATED);
}

static esp_err_t app_update_network_ota_publish_result(app_update_network_ota_handle_t handle,
                                                       app_update_network_ota_event_id_t event_id)
{
    ESP_RETURN_ON_ERROR(app_update_network_ota_post(APP_UPDATE_NETWORK_OTA_BUS_EVENT_RESULT,
                                                    &handle->result,
                                                    sizeof(handle->result)),
                        TAG,
                        "post result failed");
    return app_update_network_ota_publish_event(handle, event_id);
}

esp_err_t app_update_network_ota_init(app_update_network_ota_handle_t *out_handle)
{
    app_update_network_ota_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc network ota failed");

    handle->progress.status = APP_UPDATE_NETWORK_OTA_STATUS_IDLE;
    handle->result.result = ESP_OK;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_update_network_ota_deinit(app_update_network_ota_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_update_network_ota_start(app_update_network_ota_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    handle->progress.status = APP_UPDATE_NETWORK_OTA_STATUS_CHECKING;
    handle->progress.progress_percent = 5;
    handle->result.result = ESP_OK;
    handle->result.reboot_required = false;

    ESP_RETURN_ON_ERROR(app_update_network_ota_publish_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_STARTED),
                        TAG,
                        "post started event failed");
    ESP_RETURN_ON_ERROR(app_update_network_ota_publish_progress(handle), TAG, "post progress failed");

    // TODO: wire network package discovery and downloader once system_network upgrade path is ready.
    return ESP_OK;
}

esp_err_t app_update_network_ota_stop(app_update_network_ota_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->progress.status = APP_UPDATE_NETWORK_OTA_STATUS_IDLE;
    handle->progress.progress_percent = 0;
    return app_update_network_ota_publish_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_STOPPED);
}

esp_err_t app_update_network_ota_get_progress(app_update_network_ota_handle_t handle,
                                              app_update_network_ota_progress_t *out_progress)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_progress, ESP_ERR_INVALID_ARG, TAG, "out_progress is NULL");
    *out_progress = handle->progress;
    return ESP_OK;
}

esp_err_t app_update_network_ota_get_result(app_update_network_ota_handle_t handle,
                                            app_update_network_ota_result_t *out_result)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_result, ESP_ERR_INVALID_ARG, TAG, "out_result is NULL");
    *out_result = handle->result;
    return ESP_OK;
}
