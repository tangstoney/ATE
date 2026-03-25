#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DEVICE_PROGRAMMING_TARGET_MAX_LEN 32
#define APP_DEVICE_PROGRAMMING_STAGE_MAX_LEN 32

typedef struct app_device_programming *app_device_programming_handle_t;

ESP_EVENT_DECLARE_BASE(APP_DEVICE_PROGRAMMING_EVENT);

typedef enum {
    APP_DEVICE_PROGRAMMING_STATUS_IDLE = 0,
    APP_DEVICE_PROGRAMMING_STATUS_TARGET_SELECTED,
    APP_DEVICE_PROGRAMMING_STATUS_RUNNING,
    APP_DEVICE_PROGRAMMING_STATUS_STOPPED,
    APP_DEVICE_PROGRAMMING_STATUS_COMPLETED,
    APP_DEVICE_PROGRAMMING_STATUS_FAILED,
} app_device_programming_status_t;

typedef enum {
    APP_DEVICE_PROGRAMMING_EVENT_TARGET_SELECTED = 0,
    APP_DEVICE_PROGRAMMING_EVENT_STARTED,
    APP_DEVICE_PROGRAMMING_EVENT_PROGRESS_UPDATED,
    APP_DEVICE_PROGRAMMING_EVENT_STOPPED,
    APP_DEVICE_PROGRAMMING_EVENT_COMPLETED,
    APP_DEVICE_PROGRAMMING_EVENT_FAILED,
} app_device_programming_event_id_t;

typedef enum {
    APP_DEVICE_PROGRAMMING_BUS_EVENT_NOTIFY = 0,
    APP_DEVICE_PROGRAMMING_BUS_EVENT_PROGRESS,
    APP_DEVICE_PROGRAMMING_BUS_EVENT_RESULT,
} app_device_programming_bus_event_id_t;

typedef struct {
    app_device_programming_status_t status;
    uint8_t progress_percent;
    char stage[APP_DEVICE_PROGRAMMING_STAGE_MAX_LEN];
} app_device_programming_progress_t;

typedef struct {
    esp_err_t result;
    char selected_target[APP_DEVICE_PROGRAMMING_TARGET_MAX_LEN];
    bool retryable;
} app_device_programming_result_t;

typedef struct {
    app_device_programming_event_id_t event_id;
    app_device_programming_status_t status;
    uint8_t progress_percent;
} app_device_programming_event_t;

esp_err_t app_device_programming_init(app_device_programming_handle_t *out_handle);
esp_err_t app_device_programming_deinit(app_device_programming_handle_t handle);

esp_err_t app_device_programming_select_target(app_device_programming_handle_t handle, const char *target);
esp_err_t app_device_programming_start(app_device_programming_handle_t handle);
esp_err_t app_device_programming_stop(app_device_programming_handle_t handle);

esp_err_t app_device_programming_get_progress(app_device_programming_handle_t handle,
                                              app_device_programming_progress_t *out_progress);
esp_err_t app_device_programming_get_result(app_device_programming_handle_t handle,
                                            app_device_programming_result_t *out_result);

#ifdef __cplusplus
}
#endif
