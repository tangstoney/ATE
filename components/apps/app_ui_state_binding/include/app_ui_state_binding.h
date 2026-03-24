#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_ui_state_binding *app_ui_state_binding_handle_t;

typedef enum {
    APP_UI_STATE_BINDING_SYSTEM_STATE_UNKNOWN = 0,
    APP_UI_STATE_BINDING_SYSTEM_STATE_BOOTING,
    APP_UI_STATE_BINDING_SYSTEM_STATE_IDLE,
    APP_UI_STATE_BINDING_SYSTEM_STATE_READY,
    APP_UI_STATE_BINDING_SYSTEM_STATE_FAULT,
} app_ui_state_binding_system_state_t;

typedef enum {
    APP_UI_STATE_BINDING_RUNTIME_STATE_UNKNOWN = 0,
    APP_UI_STATE_BINDING_RUNTIME_STATE_OFFLINE,
    APP_UI_STATE_BINDING_RUNTIME_STATE_ONLINE,
    APP_UI_STATE_BINDING_RUNTIME_STATE_BUSY,
    APP_UI_STATE_BINDING_RUNTIME_STATE_FAULT,
} app_ui_state_binding_runtime_state_t;

typedef enum {
    APP_UI_STATE_BINDING_NETWORK_STATE_UNKNOWN = 0,
    APP_UI_STATE_BINDING_NETWORK_STATE_DISCONNECTED,
    APP_UI_STATE_BINDING_NETWORK_STATE_CONNECTING,
    APP_UI_STATE_BINDING_NETWORK_STATE_READY,
    APP_UI_STATE_BINDING_NETWORK_STATE_FAULT,
} app_ui_state_binding_network_state_t;

typedef enum {
    APP_UI_STATE_BINDING_TEST_STATE_IDLE = 0,
    APP_UI_STATE_BINDING_TEST_STATE_RUNNING,
    APP_UI_STATE_BINDING_TEST_STATE_PASS,
    APP_UI_STATE_BINDING_TEST_STATE_FAIL,
    APP_UI_STATE_BINDING_TEST_STATE_ABORTED,
} app_ui_state_binding_test_state_t;

typedef enum {
    APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN = 0,
    APP_UI_STATE_BINDING_UI_ICON_STATE_HIDDEN,
    APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE,
    APP_UI_STATE_BINDING_UI_ICON_STATE_ACTIVE,
    APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT,
} app_ui_state_binding_ui_icon_state_t;

typedef struct {
    app_ui_state_binding_system_state_t state;
    bool maintenance_mode;
    bool fault_active;
} app_ui_state_binding_system_state_event_t;

typedef struct {
    app_ui_state_binding_runtime_state_t state;
    uint32_t online_count;
    uint32_t total_count;
    bool rescan_in_progress;
} app_ui_state_binding_module_state_event_t;

typedef struct {
    app_ui_state_binding_runtime_state_t state;
    uint32_t online_count;
    uint32_t total_count;
    bool binding_ready;
    bool log_ready;
} app_ui_state_binding_instrument_state_event_t;

typedef struct {
    app_ui_state_binding_network_state_t state;
    bool server_connected;
    bool ip_ready;
} app_ui_state_binding_network_state_event_t;

typedef struct {
    app_ui_state_binding_test_state_t state;
    uint32_t active_step_id;
    bool context_locked;
} app_ui_state_binding_test_state_event_t;

typedef struct {
    uint32_t tick_ms;
} app_ui_state_binding_timer_tick_t;

typedef struct {
    app_ui_state_binding_ui_icon_state_t system_icon_state;
    app_ui_state_binding_ui_icon_state_t module_icon_state;
    app_ui_state_binding_ui_icon_state_t instrument_icon_state;
    app_ui_state_binding_ui_icon_state_t network_icon_state;
    app_ui_state_binding_ui_icon_state_t test_icon_state;
} app_ui_state_binding_ui_icon_snapshot_t;

typedef struct {
    app_ui_state_binding_system_state_t system_state;
    app_ui_state_binding_runtime_state_t module_state;
    app_ui_state_binding_runtime_state_t instrument_state;
    app_ui_state_binding_network_state_t network_state;
    app_ui_state_binding_test_state_t test_state;
    bool maintenance_mode;
    bool fault_active;
    bool server_connected;
    bool instrument_binding_ready;
    bool instrument_log_ready;
    bool test_context_locked;
    uint32_t module_online_count;
    uint32_t module_total_count;
    uint32_t instrument_online_count;
    uint32_t instrument_total_count;
    uint32_t active_test_step_id;
    uint32_t last_tick_ms;
} app_ui_state_binding_ui_status_snapshot_t;

typedef struct {
    uint32_t sequence;
    app_ui_state_binding_ui_icon_snapshot_t icon_snapshot;
    app_ui_state_binding_ui_status_snapshot_t status_snapshot;
} app_ui_state_binding_ui_state_update_t;

typedef esp_err_t (*app_ui_state_binding_on_ui_state_update_fn_t)(
    void *user_context,
    const app_ui_state_binding_ui_state_update_t *update);
typedef esp_err_t (*app_ui_state_binding_on_ui_icon_snapshot_fn_t)(
    void *user_context,
    const app_ui_state_binding_ui_icon_snapshot_t *icon_snapshot);
typedef esp_err_t (*app_ui_state_binding_on_ui_status_snapshot_fn_t)(
    void *user_context,
    const app_ui_state_binding_ui_status_snapshot_t *status_snapshot);

typedef struct {
    app_ui_state_binding_on_ui_state_update_fn_t on_ui_state_update;
    app_ui_state_binding_on_ui_icon_snapshot_fn_t on_ui_icon_state;
    app_ui_state_binding_on_ui_status_snapshot_fn_t on_ui_status_snapshot;
    void *user_context;
    uint32_t module_stale_timeout_ms;
    uint32_t instrument_stale_timeout_ms;
    uint32_t network_stale_timeout_ms;
} app_ui_state_binding_config_t;

esp_err_t app_ui_state_binding_init(const app_ui_state_binding_config_t *config,
                                    app_ui_state_binding_handle_t *out_handle);
esp_err_t app_ui_state_binding_start(app_ui_state_binding_handle_t handle);
esp_err_t app_ui_state_binding_stop(app_ui_state_binding_handle_t handle);
esp_err_t app_ui_state_binding_deinit(app_ui_state_binding_handle_t handle);

esp_err_t app_ui_state_binding_on_system_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_system_state_event_t *event);
esp_err_t app_ui_state_binding_on_module_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_module_state_event_t *event);
esp_err_t app_ui_state_binding_on_instrument_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_instrument_state_event_t *event);
esp_err_t app_ui_state_binding_on_network_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_network_state_event_t *event);
esp_err_t app_ui_state_binding_on_test_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_test_state_event_t *event);
esp_err_t app_ui_state_binding_on_timer_tick(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_timer_tick_t *tick);

esp_err_t app_ui_state_binding_get_ui_icon_snapshot(
    app_ui_state_binding_handle_t handle,
    app_ui_state_binding_ui_icon_snapshot_t *out_icon_snapshot);
esp_err_t app_ui_state_binding_get_ui_status_snapshot(
    app_ui_state_binding_handle_t handle,
    app_ui_state_binding_ui_status_snapshot_t *out_status_snapshot);

#ifdef __cplusplus
}
#endif
