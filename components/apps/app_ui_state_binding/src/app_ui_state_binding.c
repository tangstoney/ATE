#include "app_ui_state_binding.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

struct app_ui_state_binding {
    app_ui_state_binding_config_t config;
    app_ui_state_binding_ui_icon_snapshot_t icon_snapshot;
    app_ui_state_binding_ui_status_snapshot_t status_snapshot;
    uint32_t update_sequence;
    uint32_t last_module_event_ms;
    uint32_t last_instrument_event_ms;
    uint32_t last_network_event_ms;
    bool started;
};

static const char *TAG = "app_ui_binding";

static esp_err_t app_ui_state_binding_require_handle(app_ui_state_binding_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    return ESP_OK;
}

static esp_err_t app_ui_state_binding_require_started(app_ui_state_binding_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(handle->started, ESP_ERR_INVALID_STATE, TAG, "state binding not started");
    return ESP_OK;
}

static app_ui_state_binding_ui_icon_state_t app_ui_state_binding_map_runtime_icon(
    app_ui_state_binding_runtime_state_t state)
{
    switch (state) {
    case APP_UI_STATE_BINDING_RUNTIME_STATE_OFFLINE:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_HIDDEN;
    case APP_UI_STATE_BINDING_RUNTIME_STATE_ONLINE:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;
    case APP_UI_STATE_BINDING_RUNTIME_STATE_BUSY:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_ACTIVE;
    case APP_UI_STATE_BINDING_RUNTIME_STATE_FAULT:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT;
    case APP_UI_STATE_BINDING_RUNTIME_STATE_UNKNOWN:
    default:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }
}

static app_ui_state_binding_ui_icon_state_t app_ui_state_binding_map_network_icon(
    app_ui_state_binding_network_state_t state)
{
    switch (state) {
    case APP_UI_STATE_BINDING_NETWORK_STATE_DISCONNECTED:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_HIDDEN;
    case APP_UI_STATE_BINDING_NETWORK_STATE_CONNECTING:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_ACTIVE;
    case APP_UI_STATE_BINDING_NETWORK_STATE_READY:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;
    case APP_UI_STATE_BINDING_NETWORK_STATE_FAULT:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT;
    case APP_UI_STATE_BINDING_NETWORK_STATE_UNKNOWN:
    default:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }
}

static app_ui_state_binding_ui_icon_state_t app_ui_state_binding_map_test_icon(
    app_ui_state_binding_test_state_t state)
{
    switch (state) {
    case APP_UI_STATE_BINDING_TEST_STATE_IDLE:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;
    case APP_UI_STATE_BINDING_TEST_STATE_RUNNING:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_ACTIVE;
    case APP_UI_STATE_BINDING_TEST_STATE_FAIL:
    case APP_UI_STATE_BINDING_TEST_STATE_ABORTED:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT;
    case APP_UI_STATE_BINDING_TEST_STATE_PASS:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;
    default:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }
}

static app_ui_state_binding_ui_icon_state_t app_ui_state_binding_map_system_icon(
    app_ui_state_binding_system_state_t state,
    bool fault_active)
{
    if (fault_active) {
        return APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT;
    }

    switch (state) {
    case APP_UI_STATE_BINDING_SYSTEM_STATE_BOOTING:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_ACTIVE;
    case APP_UI_STATE_BINDING_SYSTEM_STATE_IDLE:
    case APP_UI_STATE_BINDING_SYSTEM_STATE_READY:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;
    case APP_UI_STATE_BINDING_SYSTEM_STATE_FAULT:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_FAULT;
    default:
        return APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }
}

static void app_ui_state_binding_apply_timeouts(app_ui_state_binding_handle_t handle)
{
    const uint32_t now_ms = handle->status_snapshot.last_tick_ms;

    if (handle->config.module_stale_timeout_ms > 0 &&
        handle->last_module_event_ms > 0 &&
        now_ms - handle->last_module_event_ms > handle->config.module_stale_timeout_ms) {
        handle->icon_snapshot.module_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }

    if (handle->config.instrument_stale_timeout_ms > 0 &&
        handle->last_instrument_event_ms > 0 &&
        now_ms - handle->last_instrument_event_ms > handle->config.instrument_stale_timeout_ms) {
        handle->icon_snapshot.instrument_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }

    if (handle->config.network_stale_timeout_ms > 0 &&
        handle->last_network_event_ms > 0 &&
        now_ms - handle->last_network_event_ms > handle->config.network_stale_timeout_ms) {
        handle->icon_snapshot.network_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    }
}

static esp_err_t app_ui_state_binding_emit(app_ui_state_binding_handle_t handle)
{
    app_ui_state_binding_ui_state_update_t update = {
        .sequence = ++handle->update_sequence,
        .icon_snapshot = handle->icon_snapshot,
        .status_snapshot = handle->status_snapshot,
    };

    if (handle->config.on_ui_icon_state) {
        ESP_RETURN_ON_ERROR(handle->config.on_ui_icon_state(handle->config.user_context, &handle->icon_snapshot),
                            TAG,
                            "on_ui_icon_state failed");
    }

    if (handle->config.on_ui_status_snapshot) {
        ESP_RETURN_ON_ERROR(
            handle->config.on_ui_status_snapshot(handle->config.user_context, &handle->status_snapshot),
            TAG,
            "on_ui_status_snapshot failed");
    }

    if (handle->config.on_ui_state_update) {
        ESP_RETURN_ON_ERROR(handle->config.on_ui_state_update(handle->config.user_context, &update),
                            TAG,
                            "on_ui_state_update failed");
    }

    return ESP_OK;
}

esp_err_t app_ui_state_binding_init(const app_ui_state_binding_config_t *config,
                                    app_ui_state_binding_handle_t *out_handle)
{
    app_ui_state_binding_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc state binding failed");

    if (config) {
        handle->config = *config;
    }

    handle->status_snapshot.system_state = APP_UI_STATE_BINDING_SYSTEM_STATE_UNKNOWN;
    handle->status_snapshot.module_state = APP_UI_STATE_BINDING_RUNTIME_STATE_UNKNOWN;
    handle->status_snapshot.instrument_state = APP_UI_STATE_BINDING_RUNTIME_STATE_UNKNOWN;
    handle->status_snapshot.network_state = APP_UI_STATE_BINDING_NETWORK_STATE_UNKNOWN;
    handle->status_snapshot.test_state = APP_UI_STATE_BINDING_TEST_STATE_IDLE;

    handle->icon_snapshot.system_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    handle->icon_snapshot.module_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    handle->icon_snapshot.instrument_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    handle->icon_snapshot.network_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_UNKNOWN;
    handle->icon_snapshot.test_icon_state = APP_UI_STATE_BINDING_UI_ICON_STATE_IDLE;

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_ui_state_binding_start(app_ui_state_binding_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    handle->started = true;
    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_stop(app_ui_state_binding_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    handle->started = false;
    return ESP_OK;
}

esp_err_t app_ui_state_binding_deinit(app_ui_state_binding_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    free(handle);
    return ESP_OK;
}

esp_err_t app_ui_state_binding_on_system_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_system_state_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->status_snapshot.system_state = event->state;
    handle->status_snapshot.maintenance_mode = event->maintenance_mode;
    handle->status_snapshot.fault_active = event->fault_active;
    handle->icon_snapshot.system_icon_state =
        app_ui_state_binding_map_system_icon(event->state, event->fault_active);

    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_on_module_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_module_state_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->status_snapshot.module_state = event->state;
    handle->status_snapshot.module_online_count = event->online_count;
    handle->status_snapshot.module_total_count = event->total_count;
    handle->last_module_event_ms = handle->status_snapshot.last_tick_ms;
    handle->icon_snapshot.module_icon_state = app_ui_state_binding_map_runtime_icon(event->state);

    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_on_instrument_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_instrument_state_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->status_snapshot.instrument_state = event->state;
    handle->status_snapshot.instrument_online_count = event->online_count;
    handle->status_snapshot.instrument_total_count = event->total_count;
    handle->status_snapshot.instrument_binding_ready = event->binding_ready;
    handle->status_snapshot.instrument_log_ready = event->log_ready;
    handle->last_instrument_event_ms = handle->status_snapshot.last_tick_ms;
    handle->icon_snapshot.instrument_icon_state = app_ui_state_binding_map_runtime_icon(event->state);

    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_on_network_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_network_state_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->status_snapshot.network_state = event->state;
    handle->status_snapshot.server_connected = event->server_connected;
    handle->last_network_event_ms = handle->status_snapshot.last_tick_ms;
    handle->icon_snapshot.network_icon_state = app_ui_state_binding_map_network_icon(event->state);

    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_on_test_state_event(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_test_state_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(event, ESP_ERR_INVALID_ARG, TAG, "event is NULL");

    handle->status_snapshot.test_state = event->state;
    handle->status_snapshot.active_test_step_id = event->active_step_id;
    handle->status_snapshot.test_context_locked = event->context_locked;
    handle->icon_snapshot.test_icon_state = app_ui_state_binding_map_test_icon(event->state);

    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_on_timer_tick(
    app_ui_state_binding_handle_t handle,
    const app_ui_state_binding_timer_tick_t *tick)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_started(handle), TAG, "state binding not ready");
    ESP_RETURN_ON_FALSE(tick, ESP_ERR_INVALID_ARG, TAG, "tick is NULL");

    handle->status_snapshot.last_tick_ms = tick->tick_ms;
    app_ui_state_binding_apply_timeouts(handle);
    return app_ui_state_binding_emit(handle);
}

esp_err_t app_ui_state_binding_get_ui_icon_snapshot(
    app_ui_state_binding_handle_t handle,
    app_ui_state_binding_ui_icon_snapshot_t *out_icon_snapshot)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(out_icon_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_icon_snapshot is NULL");
    *out_icon_snapshot = handle->icon_snapshot;
    return ESP_OK;
}

esp_err_t app_ui_state_binding_get_ui_status_snapshot(
    app_ui_state_binding_handle_t handle,
    app_ui_state_binding_ui_status_snapshot_t *out_status_snapshot)
{
    ESP_RETURN_ON_ERROR(app_ui_state_binding_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(out_status_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_status_snapshot is NULL");
    *out_status_snapshot = handle->status_snapshot;
    return ESP_OK;
}
