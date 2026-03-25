#include "app_device_programming.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_device_programming {
    app_device_programming_progress_t progress;
    app_device_programming_result_t result;
};

static const char *TAG = "app_dev_program";

ESP_EVENT_DEFINE_BASE(APP_DEVICE_PROGRAMMING_EVENT);

static esp_err_t app_device_programming_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_DEVICE_PROGRAMMING_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_device_programming_publish_event(app_device_programming_handle_t handle,
                                                      app_device_programming_event_id_t event_id)
{
    app_device_programming_event_t event = {
        .event_id = event_id,
        .status = handle->progress.status,
        .progress_percent = handle->progress.progress_percent,
    };
    return app_device_programming_post(APP_DEVICE_PROGRAMMING_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_device_programming_publish_progress(app_device_programming_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_device_programming_post(APP_DEVICE_PROGRAMMING_BUS_EVENT_PROGRESS,
                                                    &handle->progress,
                                                    sizeof(handle->progress)),
                        TAG,
                        "post progress failed");
    return app_device_programming_publish_event(handle, APP_DEVICE_PROGRAMMING_EVENT_PROGRESS_UPDATED);
}

static esp_err_t app_device_programming_publish_result(app_device_programming_handle_t handle,
                                                       app_device_programming_event_id_t event_id)
{
    ESP_RETURN_ON_ERROR(app_device_programming_post(APP_DEVICE_PROGRAMMING_BUS_EVENT_RESULT,
                                                    &handle->result,
                                                    sizeof(handle->result)),
                        TAG,
                        "post result failed");
    return app_device_programming_publish_event(handle, event_id);
}

esp_err_t app_device_programming_init(app_device_programming_handle_t *out_handle)
{
    app_device_programming_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc device programming failed");

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
    return app_device_programming_publish_event(handle, APP_DEVICE_PROGRAMMING_EVENT_TARGET_SELECTED);
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
    ESP_RETURN_ON_ERROR(app_device_programming_publish_event(handle, APP_DEVICE_PROGRAMMING_EVENT_STARTED),
                        TAG,
                        "post started event failed");
    ESP_RETURN_ON_ERROR(app_device_programming_publish_progress(handle), TAG, "post progress failed");

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
    ESP_RETURN_ON_ERROR(app_device_programming_publish_event(handle, APP_DEVICE_PROGRAMMING_EVENT_STOPPED),
                        TAG,
                        "post stopped event failed");
    return app_device_programming_publish_result(handle, APP_DEVICE_PROGRAMMING_EVENT_FAILED);
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
