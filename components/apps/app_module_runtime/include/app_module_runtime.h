#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_MODULE_RUNTIME_MAX_MODULES 16
#define APP_MODULE_RUNTIME_ID_MAX_LEN 32
#define APP_MODULE_RUNTIME_NAME_MAX_LEN 32

typedef struct app_module_runtime *app_module_runtime_handle_t;

ESP_EVENT_DECLARE_BASE(APP_MODULE_RUNTIME_EVENT);

typedef enum {
    APP_MODULE_RUNTIME_STATUS_UNKNOWN = 0,
    APP_MODULE_RUNTIME_STATUS_OFFLINE,
    APP_MODULE_RUNTIME_STATUS_ONLINE,
    APP_MODULE_RUNTIME_STATUS_FAULT,
} app_module_runtime_status_t;

typedef enum {
    APP_MODULE_RUNTIME_NOTIFY_ONLINE_CHANGED = 0,
    APP_MODULE_RUNTIME_NOTIFY_INFO_UPDATED,
    APP_MODULE_RUNTIME_NOTIFY_COMMAND_DONE,
} app_module_runtime_notify_id_t;

typedef enum {
    APP_MODULE_RUNTIME_BUS_EVENT_NOTIFY = 0,
    APP_MODULE_RUNTIME_BUS_EVENT_SNAPSHOT,
    APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED,
    APP_MODULE_RUNTIME_BUS_EVENT_INFO_UPDATED,
} app_module_runtime_bus_event_id_t;

typedef struct {
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    char module_type[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char display_name[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char revision[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    bool online;
    app_module_runtime_status_t status;
    uint16_t error_code;
} app_module_runtime_basic_info_t;

typedef struct {
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    bool online;
} app_module_runtime_online_changed_t;

typedef struct {
    size_t module_count;
    size_t online_count;
    app_module_runtime_basic_info_t modules[APP_MODULE_RUNTIME_MAX_MODULES];
} app_module_runtime_snapshot_t;

typedef struct {
    app_module_runtime_notify_id_t notify_id;
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    app_module_runtime_status_t status;
    bool online;
    uint16_t error_code;
    uint8_t command_id;
    bool command_success;
} app_module_runtime_event_t;

esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle);
esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle);

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot);
esp_err_t app_module_runtime_get_module_count(app_module_runtime_handle_t handle,
                                              size_t *out_count);
esp_err_t app_module_runtime_get_online_count(app_module_runtime_handle_t handle,
                                              size_t *out_count);
esp_err_t app_module_runtime_get_module_by_index(app_module_runtime_handle_t handle,
                                                 size_t index,
                                                 app_module_runtime_basic_info_t *out_info);
esp_err_t app_module_runtime_get_module_by_id(app_module_runtime_handle_t handle,
                                              const char *module_id,
                                              app_module_runtime_basic_info_t *out_info);
esp_err_t app_module_runtime_is_module_online(app_module_runtime_handle_t handle,
                                              const char *module_id,
                                              bool *out_online);

#ifdef __cplusplus
}
#endif
