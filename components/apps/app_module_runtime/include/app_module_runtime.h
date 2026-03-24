#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_MODULE_RUNTIME_MAX_MODULES 16
#define APP_MODULE_RUNTIME_ID_MAX_LEN 32
#define APP_MODULE_RUNTIME_NAME_MAX_LEN 32
#define APP_MODULE_RUNTIME_FRAME_MAX_LEN 64

typedef struct app_module_runtime *app_module_runtime_handle_t;

typedef enum {
    APP_MODULE_RUNTIME_STATUS_UNKNOWN = 0,
    APP_MODULE_RUNTIME_STATUS_DETECTED,
    APP_MODULE_RUNTIME_STATUS_ONLINE,
    APP_MODULE_RUNTIME_STATUS_OFFLINE,
    APP_MODULE_RUNTIME_STATUS_FAULT,
} app_module_runtime_status_t;

typedef enum {
    APP_MODULE_RUNTIME_EVENT_MODULE_ONLINE_CHANGED = 0,
    APP_MODULE_RUNTIME_EVENT_MODULE_INFO_UPDATED,
    APP_MODULE_RUNTIME_EVENT_MODULE_PORT_DETECTED,
    APP_MODULE_RUNTIME_EVENT_MODULE_PORT_LOST,
    APP_MODULE_RUNTIME_EVENT_MODULE_TIMEOUT,
    APP_MODULE_RUNTIME_EVENT_MODULE_MANUAL_RESCAN,
} app_module_runtime_event_id_t;

typedef struct {
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    char module_type[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char display_name[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char revision[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    bool online;
    app_module_runtime_status_t status;
} app_module_runtime_basic_info_t;

typedef struct {
    char source_tag[APP_MODULE_RUNTIME_ID_MAX_LEN];
    uint8_t payload[APP_MODULE_RUNTIME_FRAME_MAX_LEN];
    size_t len;
} app_module_runtime_rx_frame_t;

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
    app_module_runtime_event_id_t event_id;
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    app_module_runtime_status_t status;
    bool online;
} app_module_runtime_event_t;

typedef esp_err_t (*app_module_runtime_on_online_changed_fn_t)(
    void *user_context,
    const app_module_runtime_online_changed_t *online_changed);
typedef esp_err_t (*app_module_runtime_on_info_updated_fn_t)(
    void *user_context,
    const app_module_runtime_basic_info_t *basic_info);
typedef esp_err_t (*app_module_runtime_on_snapshot_fn_t)(
    void *user_context,
    const app_module_runtime_snapshot_t *snapshot);
typedef esp_err_t (*app_module_runtime_on_event_fn_t)(
    void *user_context,
    const app_module_runtime_event_t *event);

typedef struct {
    app_module_runtime_on_online_changed_fn_t on_online_changed;
    app_module_runtime_on_info_updated_fn_t on_info_updated;
    app_module_runtime_on_snapshot_fn_t on_snapshot;
    app_module_runtime_on_event_fn_t on_event;
    void *user_context;
} app_module_runtime_config_t;

esp_err_t app_module_runtime_init(const app_module_runtime_config_t *config,
                                  app_module_runtime_handle_t *out_handle);
esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle);

esp_err_t app_module_runtime_on_rx_frame(app_module_runtime_handle_t handle,
                                         const app_module_runtime_rx_frame_t *frame);
esp_err_t app_module_runtime_on_timeout(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_port_detected(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_port_lost(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_manual_rescan(app_module_runtime_handle_t handle);

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif
