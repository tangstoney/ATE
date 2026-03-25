#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_VISION_RUNTIME_TEXT_MAX_LEN 128
#define APP_VISION_RUNTIME_CONTEXT_MAX_LEN 32

typedef struct app_vision_runtime *app_vision_runtime_handle_t;

ESP_EVENT_DECLARE_BASE(APP_VISION_RUNTIME_EVENT);

typedef enum {
    APP_VISION_RUNTIME_STATUS_OFFLINE = 0,
    APP_VISION_RUNTIME_STATUS_IDLE,
    APP_VISION_RUNTIME_STATUS_RUNNING,
    APP_VISION_RUNTIME_STATUS_FAULT,
} app_vision_runtime_status_t;

typedef enum {
    APP_VISION_RUNTIME_RESULT_NONE = 0,
    APP_VISION_RUNTIME_RESULT_PENDING,
    APP_VISION_RUNTIME_RESULT_PASS,
    APP_VISION_RUNTIME_RESULT_FAIL,
} app_vision_runtime_result_code_t;

typedef enum {
    APP_VISION_RUNTIME_EVENT_ONLINE_CHANGED = 0,
    APP_VISION_RUNTIME_EVENT_MANUAL_TRIGGERED,
    APP_VISION_RUNTIME_EVENT_TEST_STEP_TRIGGERED,
    APP_VISION_RUNTIME_EVENT_VISION_PASS,
    APP_VISION_RUNTIME_EVENT_VISION_FAIL,
} app_vision_runtime_event_id_t;

typedef enum {
    APP_VISION_RUNTIME_BUS_EVENT_NOTIFY = 0,
    APP_VISION_RUNTIME_BUS_EVENT_SNAPSHOT,
    APP_VISION_RUNTIME_BUS_EVENT_STATUS,
    APP_VISION_RUNTIME_BUS_EVENT_LOG,
    APP_VISION_RUNTIME_BUS_EVENT_RESULT,
} app_vision_runtime_bus_event_id_t;

typedef struct {
    uint32_t frame_index;
    bool end_of_check;
} app_vision_runtime_frame_event_t;

typedef struct {
    char text[APP_VISION_RUNTIME_TEXT_MAX_LEN];
} app_vision_runtime_log_event_t;

typedef struct {
    char context_id[APP_VISION_RUNTIME_CONTEXT_MAX_LEN];
} app_vision_runtime_test_context_t;

typedef struct {
    uint32_t step_id;
} app_vision_runtime_test_step_trigger_t;

typedef struct {
    app_vision_runtime_result_code_t result_code;
    uint32_t frame_index;
    char detail[APP_VISION_RUNTIME_TEXT_MAX_LEN];
} app_vision_runtime_result_t;

typedef struct {
    char text[APP_VISION_RUNTIME_TEXT_MAX_LEN];
    uint32_t sequence;
} app_vision_runtime_log_record_t;

typedef struct {
    bool online;
    bool context_active;
    app_vision_runtime_status_t status;
    app_vision_runtime_result_t last_result;
} app_vision_runtime_snapshot_t;

typedef struct {
    app_vision_runtime_event_id_t event_id;
    app_vision_runtime_status_t status;
    app_vision_runtime_result_code_t result_code;
} app_vision_runtime_event_t;

esp_err_t app_vision_runtime_init(app_vision_runtime_handle_t *out_handle);
esp_err_t app_vision_runtime_deinit(app_vision_runtime_handle_t handle);

esp_err_t app_vision_runtime_on_frame_event(app_vision_runtime_handle_t handle,
                                            const app_vision_runtime_frame_event_t *event);
esp_err_t app_vision_runtime_on_log_event(app_vision_runtime_handle_t handle,
                                          const app_vision_runtime_log_event_t *event);
esp_err_t app_vision_runtime_on_device_online(app_vision_runtime_handle_t handle);
esp_err_t app_vision_runtime_on_device_offline(app_vision_runtime_handle_t handle);
esp_err_t app_vision_runtime_on_manual_trigger_check(app_vision_runtime_handle_t handle);
esp_err_t app_vision_runtime_on_test_step_trigger(app_vision_runtime_handle_t handle,
                                                  const app_vision_runtime_test_step_trigger_t *event);
esp_err_t app_vision_runtime_on_test_context_start(app_vision_runtime_handle_t handle,
                                                   const app_vision_runtime_test_context_t *context);
esp_err_t app_vision_runtime_on_test_context_stop(app_vision_runtime_handle_t handle,
                                                  const app_vision_runtime_test_context_t *context);
esp_err_t app_vision_runtime_get_snapshot(app_vision_runtime_handle_t handle,
                                          app_vision_runtime_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif
