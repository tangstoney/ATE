#include "app_feedback_runtime.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_feedback_runtime {
    app_feedback_runtime_record_t record;
};

static const char *TAG = "app_feedback_rt";

ESP_EVENT_DEFINE_BASE(APP_FEEDBACK_RUNTIME_EVENT);

static esp_err_t app_feedback_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_FEEDBACK_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_feedback_runtime_publish(app_feedback_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_feedback_runtime_post(APP_FEEDBACK_RUNTIME_BUS_EVENT_LIGHT_COMMAND,
                                                  &handle->record.light_command,
                                                  sizeof(handle->record.light_command)),
                        TAG,
                        "post light command failed");
    ESP_RETURN_ON_ERROR(app_feedback_runtime_post(APP_FEEDBACK_RUNTIME_BUS_EVENT_AUDIO_COMMAND,
                                                  &handle->record.audio_command,
                                                  sizeof(handle->record.audio_command)),
                        TAG,
                        "post audio command failed");
    return app_feedback_runtime_post(APP_FEEDBACK_RUNTIME_BUS_EVENT_RECORD_UPDATED,
                                     &handle->record,
                                     sizeof(handle->record));
}

esp_err_t app_feedback_runtime_init(app_feedback_runtime_handle_t *out_handle)
{
    app_feedback_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc feedback runtime failed");

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_feedback_runtime_deinit(app_feedback_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_feedback_runtime_on_test_event(app_feedback_runtime_handle_t handle,
                                             const app_feedback_runtime_test_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    switch (event->event_id) {
    case APP_FEEDBACK_RUNTIME_TEST_EVENT_STARTED:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_RUNNING;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_BLUE;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_PROMPT;
        break;
    case APP_FEEDBACK_RUNTIME_TEST_EVENT_PASSED:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_PASS;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_GREEN;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_SUCCESS;
        break;
    case APP_FEEDBACK_RUNTIME_TEST_EVENT_FAILED:
    case APP_FEEDBACK_RUNTIME_TEST_EVENT_ABORTED:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_FAIL;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_RED;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_ALARM;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return app_feedback_runtime_publish(handle);
}

esp_err_t app_feedback_runtime_on_system_event(app_feedback_runtime_handle_t handle,
                                               const app_feedback_runtime_system_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    switch (event->event_id) {
    case APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_READY:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_IDLE;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_GREEN;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_PROMPT;
        break;
    case APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_MAINTENANCE:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_IDLE;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_YELLOW;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_WARNING;
        break;
    case APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_IDLE:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_IDLE;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_OFF;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_NONE;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return app_feedback_runtime_publish(handle);
}

esp_err_t app_feedback_runtime_on_fault_event(app_feedback_runtime_handle_t handle,
                                              const app_feedback_runtime_fault_event_t *event)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    switch (event->event_id) {
    case APP_FEEDBACK_RUNTIME_FAULT_EVENT_WARNING:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_FAULT;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_YELLOW;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_WARNING;
        break;
    case APP_FEEDBACK_RUNTIME_FAULT_EVENT_CRITICAL:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_FAULT;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_RED;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_ALARM;
        break;
    case APP_FEEDBACK_RUNTIME_FAULT_EVENT_CLEARED:
        handle->record.state = APP_FEEDBACK_RUNTIME_RECORD_IDLE;
        handle->record.light_command = APP_FEEDBACK_RUNTIME_LIGHT_GREEN;
        handle->record.audio_command = APP_FEEDBACK_RUNTIME_AUDIO_NONE;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return app_feedback_runtime_publish(handle);
}

esp_err_t app_feedback_runtime_get_record(app_feedback_runtime_handle_t handle,
                                          app_feedback_runtime_record_t *out_record)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_record, ESP_ERR_INVALID_ARG, TAG, "out_record is NULL");
    *out_record = handle->record;
    return ESP_OK;
}
