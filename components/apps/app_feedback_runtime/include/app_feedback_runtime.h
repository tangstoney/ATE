#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_feedback_runtime *app_feedback_runtime_handle_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_LIGHT_OFF = 0,
    APP_FEEDBACK_RUNTIME_LIGHT_BLUE,
    APP_FEEDBACK_RUNTIME_LIGHT_GREEN,
    APP_FEEDBACK_RUNTIME_LIGHT_YELLOW,
    APP_FEEDBACK_RUNTIME_LIGHT_RED,
} app_feedback_runtime_light_command_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_AUDIO_NONE = 0,
    APP_FEEDBACK_RUNTIME_AUDIO_PROMPT,
    APP_FEEDBACK_RUNTIME_AUDIO_SUCCESS,
    APP_FEEDBACK_RUNTIME_AUDIO_WARNING,
    APP_FEEDBACK_RUNTIME_AUDIO_ALARM,
} app_feedback_runtime_audio_command_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_RECORD_IDLE = 0,
    APP_FEEDBACK_RUNTIME_RECORD_RUNNING,
    APP_FEEDBACK_RUNTIME_RECORD_PASS,
    APP_FEEDBACK_RUNTIME_RECORD_FAIL,
    APP_FEEDBACK_RUNTIME_RECORD_FAULT,
} app_feedback_runtime_record_state_t;

typedef struct {
    app_feedback_runtime_record_state_t state;
    app_feedback_runtime_light_command_t light_command;
    app_feedback_runtime_audio_command_t audio_command;
} app_feedback_runtime_record_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_TEST_EVENT_STARTED = 0,
    APP_FEEDBACK_RUNTIME_TEST_EVENT_PASSED,
    APP_FEEDBACK_RUNTIME_TEST_EVENT_FAILED,
    APP_FEEDBACK_RUNTIME_TEST_EVENT_ABORTED,
} app_feedback_runtime_test_event_id_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_READY = 0,
    APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_MAINTENANCE,
    APP_FEEDBACK_RUNTIME_SYSTEM_EVENT_IDLE,
} app_feedback_runtime_system_event_id_t;

typedef enum {
    APP_FEEDBACK_RUNTIME_FAULT_EVENT_WARNING = 0,
    APP_FEEDBACK_RUNTIME_FAULT_EVENT_CRITICAL,
    APP_FEEDBACK_RUNTIME_FAULT_EVENT_CLEARED,
} app_feedback_runtime_fault_event_id_t;

typedef struct {
    app_feedback_runtime_test_event_id_t event_id;
} app_feedback_runtime_test_event_t;

typedef struct {
    app_feedback_runtime_system_event_id_t event_id;
} app_feedback_runtime_system_event_t;

typedef struct {
    app_feedback_runtime_fault_event_id_t event_id;
} app_feedback_runtime_fault_event_t;

typedef esp_err_t (*app_feedback_runtime_on_light_command_fn_t)(
    void *user_context,
    app_feedback_runtime_light_command_t light_command);
typedef esp_err_t (*app_feedback_runtime_on_audio_command_fn_t)(
    void *user_context,
    app_feedback_runtime_audio_command_t audio_command);
typedef esp_err_t (*app_feedback_runtime_on_record_fn_t)(
    void *user_context,
    const app_feedback_runtime_record_t *record);

typedef struct {
    app_feedback_runtime_on_light_command_fn_t on_light_command;
    app_feedback_runtime_on_audio_command_fn_t on_audio_command;
    app_feedback_runtime_on_record_fn_t on_record;
    void *user_context;
} app_feedback_runtime_config_t;

esp_err_t app_feedback_runtime_init(const app_feedback_runtime_config_t *config,
                                    app_feedback_runtime_handle_t *out_handle);
esp_err_t app_feedback_runtime_deinit(app_feedback_runtime_handle_t handle);

esp_err_t app_feedback_runtime_on_test_event(app_feedback_runtime_handle_t handle,
                                             const app_feedback_runtime_test_event_t *event);
esp_err_t app_feedback_runtime_on_system_event(app_feedback_runtime_handle_t handle,
                                               const app_feedback_runtime_system_event_t *event);
esp_err_t app_feedback_runtime_on_fault_event(app_feedback_runtime_handle_t handle,
                                              const app_feedback_runtime_fault_event_t *event);
esp_err_t app_feedback_runtime_get_record(app_feedback_runtime_handle_t handle,
                                          app_feedback_runtime_record_t *out_record);

#ifdef __cplusplus
}
#endif
