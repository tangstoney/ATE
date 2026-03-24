#include "app_update_network_ota.h"

#include <stdlib.h>

#include "esp_check.h"

struct app_update_network_ota {
    app_update_network_ota_config_t config;
    app_update_network_ota_progress_t progress;
    app_update_network_ota_result_t result;
};

static const char *TAG = "app_upd_net_ota";

static esp_err_t app_update_network_ota_emit_event(app_update_network_ota_handle_t handle,
                                                   app_update_network_ota_event_id_t event_id)
{
    app_update_network_ota_event_t event = {
        .event_id = event_id,
        .status = handle->progress.status,
        .progress_percent = handle->progress.progress_percent,
        .result = handle->result.result,
    };

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event), TAG, "on_event failed");
    }
    return ESP_OK;
}

static esp_err_t app_update_network_ota_emit_progress(app_update_network_ota_handle_t handle)
{
    if (handle->config.on_progress) {
        ESP_RETURN_ON_ERROR(handle->config.on_progress(handle->config.user_context, &handle->progress),
                            TAG,
                            "on_progress failed");
    }
    return app_update_network_ota_emit_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_PROGRESS_UPDATED);
}

static esp_err_t app_update_network_ota_emit_result(app_update_network_ota_handle_t handle,
                                                    app_update_network_ota_event_id_t event_id)
{
    if (handle->config.on_result) {
        ESP_RETURN_ON_ERROR(handle->config.on_result(handle->config.user_context, &handle->result),
                            TAG,
                            "on_result failed");
    }
    return app_update_network_ota_emit_event(handle, event_id);
}

esp_err_t app_update_network_ota_init(const app_update_network_ota_config_t *config,
                                      app_update_network_ota_handle_t *out_handle)
{
    app_update_network_ota_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc network ota failed");

    if (config) {
        handle->config = *config;
    }

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

    ESP_RETURN_ON_ERROR(app_update_network_ota_emit_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_STARTED),
                        TAG,
                        "emit started event failed");
    ESP_RETURN_ON_ERROR(app_update_network_ota_emit_progress(handle), TAG, "emit progress failed");

    // TODO: wire network package discovery and downloader once system_network upgrade path is ready.
    return ESP_OK;
}

esp_err_t app_update_network_ota_stop(app_update_network_ota_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->progress.status = APP_UPDATE_NETWORK_OTA_STATUS_IDLE;
    handle->progress.progress_percent = 0;
    return app_update_network_ota_emit_event(handle, APP_UPDATE_NETWORK_OTA_EVENT_STOPPED);
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
