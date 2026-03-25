#include "app_vision_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_vision_runtime {
    app_vision_runtime_snapshot_t snapshot;
    uint32_t log_sequence;
};

static const char *TAG = "app_vision_runtime";

ESP_EVENT_DEFINE_BASE(APP_VISION_RUNTIME_EVENT);

static esp_err_t app_vision_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_VISION_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_vision_runtime_publish_snapshot(app_vision_runtime_handle_t handle)
{
    return app_vision_runtime_post(APP_VISION_RUNTIME_BUS_EVENT_SNAPSHOT,
                                   &handle->snapshot,
                                   sizeof(handle->snapshot));
}

static esp_err_t app_vision_runtime_publish_status(app_vision_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_vision_runtime_post(APP_VISION_RUNTIME_BUS_EVENT_STATUS,
                                                &handle->snapshot.status,
                                                sizeof(handle->snapshot.status)),
                        TAG,
                        "post status failed");
    return app_vision_runtime_publish_snapshot(handle);
}

static esp_err_t app_vision_runtime_publish_event(app_vision_runtime_handle_t handle,
                                                  app_vision_runtime_event_id_t event_id)
{
    app_vision_runtime_event_t event = {
        .event_id = event_id,
        .status = handle->snapshot.status,
        .result_code = handle->snapshot.last_result.result_code,
    };

    return app_vision_runtime_post(APP_VISION_RUNTIME_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_vision_runtime_publish_result(app_vision_runtime_handle_t handle,
                                                   app_vision_runtime_event_id_t event_id)
{
    ESP_RETURN_ON_ERROR(app_vision_runtime_post(APP_VISION_RUNTIME_BUS_EVENT_RESULT,
                                                &handle->snapshot.last_result,
                                                sizeof(handle->snapshot.last_result)),
                        TAG,
                        "post result failed");
    ESP_RETURN_ON_ERROR(app_vision_runtime_publish_event(handle, event_id), TAG, "post result event failed");
    return app_vision_runtime_publish_snapshot(handle);
}

esp_err_t app_vision_runtime_init(app_vision_runtime_handle_t *out_handle)
{
    app_vision_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc vision runtime failed");

    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_OFFLINE;
    handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_NONE;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_vision_runtime_deinit(app_vision_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_vision_runtime_on_frame_event(app_vision_runtime_handle_t handle,
                                            const app_vision_runtime_frame_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_RUNNING;
    handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_PENDING;
    handle->snapshot.last_result.frame_index = event->frame_index;
    snprintf(handle->snapshot.last_result.detail, sizeof(handle->snapshot.last_result.detail), "%s", "frame_received");

    if (event->end_of_check) {
        handle->snapshot.status = APP_VISION_RUNTIME_STATUS_IDLE;
    }

    return app_vision_runtime_publish_status(handle);
}

esp_err_t app_vision_runtime_on_log_event(app_vision_runtime_handle_t handle,
                                          const app_vision_runtime_log_event_t *event)
{
    app_vision_runtime_log_record_t log_record = {0};

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    log_record.sequence = ++handle->log_sequence;
    snprintf(log_record.text, sizeof(log_record.text), "%s", event->text);

    ESP_RETURN_ON_ERROR(app_vision_runtime_post(APP_VISION_RUNTIME_BUS_EVENT_LOG,
                                                &log_record,
                                                sizeof(log_record)),
                        TAG,
                        "post log failed");

    if (strstr(event->text, "PASS")) {
        handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_PASS;
        snprintf(handle->snapshot.last_result.detail, sizeof(handle->snapshot.last_result.detail), "%s", event->text);
        return app_vision_runtime_publish_result(handle, APP_VISION_RUNTIME_EVENT_VISION_PASS);
    }

    if (strstr(event->text, "FAIL")) {
        handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_FAIL;
        snprintf(handle->snapshot.last_result.detail, sizeof(handle->snapshot.last_result.detail), "%s", event->text);
        return app_vision_runtime_publish_result(handle, APP_VISION_RUNTIME_EVENT_VISION_FAIL);
    }

    return ESP_OK;
}

esp_err_t app_vision_runtime_on_device_online(app_vision_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->snapshot.online = true;
    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_IDLE;
    ESP_RETURN_ON_ERROR(app_vision_runtime_publish_status(handle), TAG, "post status failed");
    return app_vision_runtime_publish_event(handle, APP_VISION_RUNTIME_EVENT_ONLINE_CHANGED);
}

esp_err_t app_vision_runtime_on_device_offline(app_vision_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->snapshot.online = false;
    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_OFFLINE;
    ESP_RETURN_ON_ERROR(app_vision_runtime_publish_status(handle), TAG, "post status failed");
    return app_vision_runtime_publish_event(handle, APP_VISION_RUNTIME_EVENT_ONLINE_CHANGED);
}

esp_err_t app_vision_runtime_on_manual_trigger_check(app_vision_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_RUNNING;
    handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_PENDING;
    snprintf(handle->snapshot.last_result.detail, sizeof(handle->snapshot.last_result.detail), "%s", "manual_trigger");
    ESP_RETURN_ON_ERROR(app_vision_runtime_publish_status(handle), TAG, "post status failed");
    return app_vision_runtime_publish_event(handle, APP_VISION_RUNTIME_EVENT_MANUAL_TRIGGERED);
}

esp_err_t app_vision_runtime_on_test_step_trigger(app_vision_runtime_handle_t handle,
                                                  const app_vision_runtime_test_step_trigger_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->snapshot.status = APP_VISION_RUNTIME_STATUS_RUNNING;
    handle->snapshot.last_result.result_code = APP_VISION_RUNTIME_RESULT_PENDING;
    snprintf(handle->snapshot.last_result.detail,
             sizeof(handle->snapshot.last_result.detail),
             "step_%lu_trigger",
             (unsigned long)event->step_id);
    ESP_RETURN_ON_ERROR(app_vision_runtime_publish_status(handle), TAG, "post status failed");
    return app_vision_runtime_publish_event(handle, APP_VISION_RUNTIME_EVENT_TEST_STEP_TRIGGERED);
}

esp_err_t app_vision_runtime_on_test_context_start(app_vision_runtime_handle_t handle,
                                                   const app_vision_runtime_test_context_t *context)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "context is NULL");
    handle->snapshot.context_active = true;
    return app_vision_runtime_publish_snapshot(handle);
}

esp_err_t app_vision_runtime_on_test_context_stop(app_vision_runtime_handle_t handle,
                                                  const app_vision_runtime_test_context_t *context)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "context is NULL");
    handle->snapshot.context_active = false;
    handle->snapshot.status = handle->snapshot.online ? APP_VISION_RUNTIME_STATUS_IDLE
                                                      : APP_VISION_RUNTIME_STATUS_OFFLINE;
    return app_vision_runtime_publish_snapshot(handle);
}

esp_err_t app_vision_runtime_get_snapshot(app_vision_runtime_handle_t handle,
                                          app_vision_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}
