#include "app_device_programming.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

struct app_device_programming {
    app_device_programming_config_t config;
    app_device_programming_progress_t progress;
    app_device_programming_result_t result;
};

static const char *TAG = "app_dev_program";

static esp_err_t app_device_programming_emit_event(app_device_programming_handle_t handle,
                                                   app_device_programming_event_id_t event_id)
{
    app_device_programming_event_t event = {
        .event_id = event_id,
        .status = handle->progress.status,
        .progress_percent = handle->progress.progress_percent,
    };

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event), TAG, "on_event failed");
    }
    return ESP_OK;
}

static esp_err_t app_device_programming_emit_progress(app_device_programming_handle_t handle)
{
    if (handle->config.on_progress) {
        ESP_RETURN_ON_ERROR(handle->config.on_progress(handle->config.user_context, &handle->progress),
                            TAG,
                            "on_progress failed");
    }
    return app_device_programming_emit_event(handle, APP_DEVICE_PROGRAMMING_EVENT_PROGRESS_UPDATED);
}

static esp_err_t app_device_programming_emit_result(app_device_programming_handle_t handle,
                                                    app_device_programming_event_id_t event_id)
{
    if (handle->config.on_result) {
        ESP_RETURN_ON_ERROR(handle->config.on_result(handle->config.user_context, &handle->result),
                            TAG,
                            "on_result failed");
    }
    return app_device_programming_emit_event(handle, event_id);
}

esp_err_t app_device_programming_init(const app_device_programming_config_t *config,
                                      app_device_programming_handle_t *out_handle)
{
    app_device_programming_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc device programming failed");

    if (config) {
        handle->config = *config;
    }

    handle->progress.status = APP_DEVICE_PROGRAMMING_STATUS_IDLE;
    handle->result.result = ESP_OK;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_device_programming_deinit(app_device_programming_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_device_programming_select_target(app_device_programming_handle_t handle, const char *target)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(target, ESP_ERR_INVALID_ARG, TAG, "target is NULL");

    snprintf(handle->result.selected_target, sizeof(handle->result.selected_target), "%s", target);
    handle->progress.status = APP_DEVICE_PROGRAMMING_STATUS_TARGET_SELECTED;
    snprintf(handle->progress.stage, sizeof(handle->progress.stage), "%s", "target_selected");
    handle->progress.progress_percent = 0;
    return app_device_programming_emit_event(handle, APP_DEVICE_PROGRAMMING_EVENT_TARGET_SELECTED);
}

esp_err_t app_device_programming_start(app_device_programming_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(handle->result.selected_target[0] != '\0',
                        ESP_ERR_INVALID_STATE,
                        TAG,
                        "target not selected");

    handle->progress.status = APP_DEVICE_PROGRAMMING_STATUS_RUNNING;
    handle->progress.progress_percent = 5;
    snprintf(handle->progress.stage, sizeof(handle->progress.stage), "%s", "session_started");
    ESP_RETURN_ON_ERROR(app_device_programming_emit_event(handle, APP_DEVICE_PROGRAMMING_EVENT_STARTED),
                        TAG,
                        "emit started event failed");
    ESP_RETURN_ON_ERROR(app_device_programming_emit_progress(handle), TAG, "emit progress failed");

    // TODO: connect firmware_provider + programming_executor after system layer is finalized.
    return ESP_OK;
}

esp_err_t app_device_programming_stop(app_device_programming_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    handle->progress.status = APP_DEVICE_PROGRAMMING_STATUS_STOPPED;
    snprintf(handle->progress.stage, sizeof(handle->progress.stage), "%s", "stopped");
    handle->result.result = ESP_ERR_INVALID_STATE;
    handle->result.retryable = true;
    ESP_RETURN_ON_ERROR(app_device_programming_emit_event(handle, APP_DEVICE_PROGRAMMING_EVENT_STOPPED),
                        TAG,
                        "emit stopped event failed");
    return app_device_programming_emit_result(handle, APP_DEVICE_PROGRAMMING_EVENT_FAILED);
}

esp_err_t app_device_programming_get_progress(app_device_programming_handle_t handle,
                                              app_device_programming_progress_t *out_progress)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_progress, ESP_ERR_INVALID_ARG, TAG, "out_progress is NULL");
    *out_progress = handle->progress;
    return ESP_OK;
}

esp_err_t app_device_programming_get_result(app_device_programming_handle_t handle,
                                            app_device_programming_result_t *out_result)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_result, ESP_ERR_INVALID_ARG, TAG, "out_result is NULL");
    *out_result = handle->result;
    return ESP_OK;
}
