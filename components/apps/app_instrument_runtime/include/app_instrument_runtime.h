#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_INSTRUMENT_RUNTIME_MAX_INSTRUMENTS 8
#define APP_INSTRUMENT_RUNTIME_ID_MAX_LEN 32
#define APP_INSTRUMENT_RUNTIME_TEXT_MAX_LEN 128

typedef struct app_instrument_runtime *app_instrument_runtime_handle_t;

typedef enum {
    APP_INSTRUMENT_RUNTIME_STATUS_UNKNOWN = 0,
    APP_INSTRUMENT_RUNTIME_STATUS_OFFLINE,
    APP_INSTRUMENT_RUNTIME_STATUS_ONLINE,
    APP_INSTRUMENT_RUNTIME_STATUS_ACTIVE,
    APP_INSTRUMENT_RUNTIME_STATUS_FAULT,
} app_instrument_runtime_status_t;

typedef enum {
    APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_ADDED = 0,
    APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_REMOVED,
    APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_UPDATED,
    APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_LOG_READY,
    APP_INSTRUMENT_RUNTIME_EVENT_MANUAL_RESCAN_REQUESTED,
    APP_INSTRUMENT_RUNTIME_EVENT_MANUAL_CLEAR_BINDING,
    APP_INSTRUMENT_RUNTIME_EVENT_ATE_CONTEXT_STARTED,
    APP_INSTRUMENT_RUNTIME_EVENT_ATE_CONTEXT_STOPPED,
} app_instrument_runtime_event_id_t;

typedef struct {
    char instrument_id[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char instrument_type[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char display_name[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char binding_label[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    bool online;
    app_instrument_runtime_status_t status;
} app_instrument_runtime_basic_info_t;

typedef struct {
    char instrument_id[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char port_tag[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char log_text[APP_INSTRUMENT_RUNTIME_TEXT_MAX_LEN];
    uint32_t sequence;
    bool context_frozen;
} app_instrument_runtime_log_record_t;

typedef struct {
    size_t instrument_count;
    size_t online_count;
    bool context_frozen;
    app_instrument_runtime_basic_info_t instruments[APP_INSTRUMENT_RUNTIME_MAX_INSTRUMENTS];
} app_instrument_runtime_snapshot_t;

typedef struct {
    char port_tag[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    char text[APP_INSTRUMENT_RUNTIME_TEXT_MAX_LEN];
} app_instrument_runtime_rx_text_t;

typedef struct {
    char context_name[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    bool freeze_current_binding;
} app_instrument_runtime_ate_context_t;

typedef struct {
    char instrument_id[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    bool online;
} app_instrument_runtime_online_changed_t;

typedef struct {
    app_instrument_runtime_event_id_t event_id;
    char instrument_id[APP_INSTRUMENT_RUNTIME_ID_MAX_LEN];
    app_instrument_runtime_status_t status;
} app_instrument_runtime_event_t;

typedef esp_err_t (*app_instrument_runtime_on_snapshot_fn_t)(
    void *user_context,
    const app_instrument_runtime_snapshot_t *snapshot);
typedef esp_err_t (*app_instrument_runtime_on_online_changed_fn_t)(
    void *user_context,
    const app_instrument_runtime_online_changed_t *online_changed);
typedef esp_err_t (*app_instrument_runtime_on_info_changed_fn_t)(
    void *user_context,
    const app_instrument_runtime_basic_info_t *basic_info);
typedef esp_err_t (*app_instrument_runtime_on_log_ready_fn_t)(
    void *user_context,
    const app_instrument_runtime_log_record_t *log_record);
typedef esp_err_t (*app_instrument_runtime_on_event_fn_t)(
    void *user_context,
    const app_instrument_runtime_event_t *event);

typedef struct {
    app_instrument_runtime_on_snapshot_fn_t on_snapshot;
    app_instrument_runtime_on_online_changed_fn_t on_online_changed;
    app_instrument_runtime_on_info_changed_fn_t on_info_changed;
    app_instrument_runtime_on_log_ready_fn_t on_log_ready;
    app_instrument_runtime_on_event_fn_t on_event;
    void *user_context;
} app_instrument_runtime_config_t;

esp_err_t app_instrument_runtime_init(const app_instrument_runtime_config_t *config,
                                      app_instrument_runtime_handle_t *out_handle);
esp_err_t app_instrument_runtime_start(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_stop(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_deinit(app_instrument_runtime_handle_t handle);

esp_err_t app_instrument_runtime_on_scan_tick(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_on_rx_text(app_instrument_runtime_handle_t handle,
                                            const app_instrument_runtime_rx_text_t *rx_text);
esp_err_t app_instrument_runtime_on_port_active(app_instrument_runtime_handle_t handle, const char *instrument_id);
esp_err_t app_instrument_runtime_on_port_lost(app_instrument_runtime_handle_t handle, const char *instrument_id);
esp_err_t app_instrument_runtime_on_manual_rescan(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_on_manual_clear_binding(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_on_ate_start_context(app_instrument_runtime_handle_t handle,
                                                      const app_instrument_runtime_ate_context_t *context);
esp_err_t app_instrument_runtime_on_ate_stop_context(app_instrument_runtime_handle_t handle,
                                                     const app_instrument_runtime_ate_context_t *context);
esp_err_t app_instrument_runtime_freeze_context(app_instrument_runtime_handle_t handle);
esp_err_t app_instrument_runtime_restore_context(app_instrument_runtime_handle_t handle);

esp_err_t app_instrument_runtime_get_snapshot(app_instrument_runtime_handle_t handle,
                                              app_instrument_runtime_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif
