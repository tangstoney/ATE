#include "app_instrument_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

struct app_instrument_runtime {
    app_instrument_runtime_config_t config;
    app_instrument_runtime_snapshot_t snapshot;
    app_instrument_runtime_snapshot_t frozen_snapshot;
    uint32_t log_sequence;
    bool started;
};

static const char *TAG = "app_instr_runtime";

static esp_err_t app_instrument_runtime_require_started(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(handle->started, ESP_ERR_INVALID_STATE, TAG, "instrument runtime not started");
    return ESP_OK;
}

static app_instrument_runtime_basic_info_t *app_instrument_runtime_find_slot(
    app_instrument_runtime_handle_t handle,
    const char *instrument_id,
    bool create_if_missing)
{
    size_t i = 0;

    for (i = 0; i < handle->snapshot.instrument_count; ++i) {
        if (strncmp(handle->snapshot.instruments[i].instrument_id,
                    instrument_id,
                    sizeof(handle->snapshot.instruments[i].instrument_id)) == 0) {
            return &handle->snapshot.instruments[i];
        }
    }

    if (!create_if_missing || handle->snapshot.instrument_count >= APP_INSTRUMENT_RUNTIME_MAX_INSTRUMENTS) {
        return NULL;
    }

    i = handle->snapshot.instrument_count++;
    snprintf(handle->snapshot.instruments[i].instrument_id, sizeof(handle->snapshot.instruments[i].instrument_id), "%s", instrument_id);
    snprintf(handle->snapshot.instruments[i].instrument_type, sizeof(handle->snapshot.instruments[i].instrument_type), "%s", "unidentified");
    snprintf(handle->snapshot.instruments[i].display_name, sizeof(handle->snapshot.instruments[i].display_name), "%s", instrument_id);
    handle->snapshot.instruments[i].status = APP_INSTRUMENT_RUNTIME_STATUS_UNKNOWN;
    return &handle->snapshot.instruments[i];
}

static void app_instrument_runtime_refresh_online_count(app_instrument_runtime_handle_t handle)
{
    size_t i = 0;
    handle->snapshot.online_count = 0;
    for (i = 0; i < handle->snapshot.instrument_count; ++i) {
        if (handle->snapshot.instruments[i].online) {
            ++handle->snapshot.online_count;
        }
    }
}

static esp_err_t app_instrument_runtime_emit_snapshot(app_instrument_runtime_handle_t handle)
{
    if (handle->config.on_snapshot) {
        return handle->config.on_snapshot(handle->config.user_context, &handle->snapshot);
    }
    return ESP_OK;
}

static esp_err_t app_instrument_runtime_emit_event(app_instrument_runtime_handle_t handle,
                                                   app_instrument_runtime_event_id_t event_id,
                                                   const app_instrument_runtime_basic_info_t *slot)
{
    app_instrument_runtime_event_t event = {
        .event_id = event_id,
        .status = slot ? slot->status : APP_INSTRUMENT_RUNTIME_STATUS_UNKNOWN,
    };

    if (slot) {
        snprintf(event.instrument_id, sizeof(event.instrument_id), "%s", slot->instrument_id);
    }

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event), TAG, "on_event failed");
    }
    return ESP_OK;
}

static esp_err_t app_instrument_runtime_emit_online(app_instrument_runtime_handle_t handle,
                                                    const app_instrument_runtime_basic_info_t *slot)
{
    app_instrument_runtime_online_changed_t online_changed = {
        .online = slot->online,
    };

    snprintf(online_changed.instrument_id, sizeof(online_changed.instrument_id), "%s", slot->instrument_id);
    if (handle->config.on_online_changed) {
        ESP_RETURN_ON_ERROR(handle->config.on_online_changed(handle->config.user_context, &online_changed),
                            TAG,
                            "on_online_changed failed");
    }
    return ESP_OK;
}

static esp_err_t app_instrument_runtime_emit_info(app_instrument_runtime_handle_t handle,
                                                  const app_instrument_runtime_basic_info_t *slot)
{
    if (handle->config.on_info_changed) {
        ESP_RETURN_ON_ERROR(handle->config.on_info_changed(handle->config.user_context, slot),
                            TAG,
                            "on_info_changed failed");
    }
    return ESP_OK;
}

esp_err_t app_instrument_runtime_init(const app_instrument_runtime_config_t *config,
                                      app_instrument_runtime_handle_t *out_handle)
{
    app_instrument_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc instrument runtime failed");

    if (config) {
        handle->config = *config;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_instrument_runtime_start(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->started = true;
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_stop(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->started = false;
    return ESP_OK;
}

esp_err_t app_instrument_runtime_deinit(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_instrument_runtime_on_scan_tick(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_rx_text(app_instrument_runtime_handle_t handle,
                                            const app_instrument_runtime_rx_text_t *rx_text)
{
    app_instrument_runtime_basic_info_t *slot = NULL;
    app_instrument_runtime_log_record_t log_record = {0};

    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_FALSE(rx_text && rx_text->port_tag[0] != '\0', ESP_ERR_INVALID_ARG, TAG, "invalid rx_text");

    slot = app_instrument_runtime_find_slot(handle, rx_text->port_tag, true);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no instrument slot available");

    slot->online = true;
    slot->status = APP_INSTRUMENT_RUNTIME_STATUS_ACTIVE;
    snprintf(slot->binding_label, sizeof(slot->binding_label), "%s", rx_text->port_tag);
    app_instrument_runtime_refresh_online_count(handle);

    log_record.sequence = ++handle->log_sequence;
    log_record.context_frozen = handle->snapshot.context_frozen;
    snprintf(log_record.instrument_id, sizeof(log_record.instrument_id), "%s", slot->instrument_id);
    snprintf(log_record.port_tag, sizeof(log_record.port_tag), "%s", rx_text->port_tag);
    snprintf(log_record.log_text, sizeof(log_record.log_text), "%s", rx_text->text);

    if (handle->config.on_log_ready) {
        ESP_RETURN_ON_ERROR(handle->config.on_log_ready(handle->config.user_context, &log_record),
                            TAG,
                            "on_log_ready failed");
    }

    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_info(handle, slot), TAG, "emit info failed");
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_LOG_READY, slot),
                        TAG,
                        "emit log event failed");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_port_active(app_instrument_runtime_handle_t handle, const char *instrument_id)
{
    app_instrument_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_FALSE(instrument_id, ESP_ERR_INVALID_ARG, TAG, "instrument_id is NULL");

    slot = app_instrument_runtime_find_slot(handle, instrument_id, true);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no instrument slot available");

    slot->online = true;
    slot->status = APP_INSTRUMENT_RUNTIME_STATUS_ONLINE;
    app_instrument_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_online(handle, slot), TAG, "emit online failed");
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_ADDED, slot),
                        TAG,
                        "emit add event failed");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_port_lost(app_instrument_runtime_handle_t handle, const char *instrument_id)
{
    app_instrument_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_FALSE(instrument_id, ESP_ERR_INVALID_ARG, TAG, "instrument_id is NULL");

    slot = app_instrument_runtime_find_slot(handle, instrument_id, false);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NOT_FOUND, TAG, "instrument not found");

    slot->online = false;
    slot->status = APP_INSTRUMENT_RUNTIME_STATUS_OFFLINE;
    app_instrument_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_online(handle, slot), TAG, "emit online failed");
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_INSTRUMENT_REMOVED, slot),
                        TAG,
                        "emit remove event failed");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_manual_rescan(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_MANUAL_RESCAN_REQUESTED, NULL),
                        TAG,
                        "emit rescan event failed");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_manual_clear_binding(app_instrument_runtime_handle_t handle)
{
    size_t i = 0;

    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");

    for (i = 0; i < handle->snapshot.instrument_count; ++i) {
        handle->snapshot.instruments[i].binding_label[0] = '\0';
    }

    ESP_RETURN_ON_ERROR(app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_MANUAL_CLEAR_BINDING, NULL),
                        TAG,
                        "emit clear binding event failed");
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_freeze_context(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    handle->frozen_snapshot = handle->snapshot;
    handle->snapshot.context_frozen = true;
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_restore_context(app_instrument_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    handle->snapshot = handle->frozen_snapshot;
    handle->snapshot.context_frozen = false;
    return app_instrument_runtime_emit_snapshot(handle);
}

esp_err_t app_instrument_runtime_on_ate_start_context(app_instrument_runtime_handle_t handle,
                                                      const app_instrument_runtime_ate_context_t *context)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "context is NULL");

    if (context->freeze_current_binding) {
        ESP_RETURN_ON_ERROR(app_instrument_runtime_freeze_context(handle), TAG, "freeze context failed");
    }

    return app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_ATE_CONTEXT_STARTED, NULL);
}

esp_err_t app_instrument_runtime_on_ate_stop_context(app_instrument_runtime_handle_t handle,
                                                     const app_instrument_runtime_ate_context_t *context)
{
    ESP_RETURN_ON_ERROR(app_instrument_runtime_require_started(handle), TAG, "instrument runtime not ready");
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "context is NULL");
    ESP_RETURN_ON_ERROR(app_instrument_runtime_restore_context(handle), TAG, "restore context failed");
    return app_instrument_runtime_emit_event(handle, APP_INSTRUMENT_RUNTIME_EVENT_ATE_CONTEXT_STOPPED, NULL);
}

esp_err_t app_instrument_runtime_get_snapshot(app_instrument_runtime_handle_t handle,
                                              app_instrument_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}
