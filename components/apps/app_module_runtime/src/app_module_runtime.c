#include "app_module_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"

struct app_module_runtime {
    app_module_runtime_config_t config;
    app_module_runtime_snapshot_t snapshot;
    bool started;
};

static const char *TAG = "app_module_runtime";

static esp_err_t app_module_runtime_require_started(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(handle->started, ESP_ERR_INVALID_STATE, TAG, "module runtime not started");
    return ESP_OK;
}

static app_module_runtime_basic_info_t *app_module_runtime_find_slot(app_module_runtime_handle_t handle,
                                                                     const char *module_id,
                                                                     bool create_if_missing)
{
    size_t i = 0;

    for (i = 0; i < handle->snapshot.module_count; ++i) {
        if (strncmp(handle->snapshot.modules[i].module_id, module_id, sizeof(handle->snapshot.modules[i].module_id)) == 0) {
            return &handle->snapshot.modules[i];
        }
    }

    if (!create_if_missing || handle->snapshot.module_count >= APP_MODULE_RUNTIME_MAX_MODULES) {
        return NULL;
    }

    i = handle->snapshot.module_count++;
    snprintf(handle->snapshot.modules[i].module_id, sizeof(handle->snapshot.modules[i].module_id), "%s", module_id);
    snprintf(handle->snapshot.modules[i].module_type, sizeof(handle->snapshot.modules[i].module_type), "%s", "unidentified");
    snprintf(handle->snapshot.modules[i].display_name, sizeof(handle->snapshot.modules[i].display_name), "%s", module_id);
    snprintf(handle->snapshot.modules[i].revision, sizeof(handle->snapshot.modules[i].revision), "%s", "pending");
    handle->snapshot.modules[i].status = APP_MODULE_RUNTIME_STATUS_DETECTED;
    return &handle->snapshot.modules[i];
}

static void app_module_runtime_refresh_online_count(app_module_runtime_handle_t handle)
{
    size_t i = 0;
    handle->snapshot.online_count = 0;
    for (i = 0; i < handle->snapshot.module_count; ++i) {
        if (handle->snapshot.modules[i].online) {
            ++handle->snapshot.online_count;
        }
    }
}

static esp_err_t app_module_runtime_emit_snapshot(app_module_runtime_handle_t handle)
{
    if (handle->config.on_snapshot) {
        return handle->config.on_snapshot(handle->config.user_context, &handle->snapshot);
    }
    return ESP_OK;
}

static esp_err_t app_module_runtime_emit_event(app_module_runtime_handle_t handle,
                                               app_module_runtime_event_id_t event_id,
                                               const app_module_runtime_basic_info_t *slot)
{
    app_module_runtime_event_t event = {
        .event_id = event_id,
        .status = slot ? slot->status : APP_MODULE_RUNTIME_STATUS_UNKNOWN,
        .online = slot ? slot->online : false,
    };

    if (slot) {
        snprintf(event.module_id, sizeof(event.module_id), "%s", slot->module_id);
    }

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event), TAG, "on_event failed");
    }

    return ESP_OK;
}

static esp_err_t app_module_runtime_emit_info(app_module_runtime_handle_t handle,
                                              const app_module_runtime_basic_info_t *slot)
{
    if (handle->config.on_info_updated) {
        ESP_RETURN_ON_ERROR(handle->config.on_info_updated(handle->config.user_context, slot),
                            TAG,
                            "on_info_updated failed");
    }
    return ESP_OK;
}

static esp_err_t app_module_runtime_emit_online(app_module_runtime_handle_t handle,
                                                const app_module_runtime_basic_info_t *slot)
{
    app_module_runtime_online_changed_t online_changed = {
        .online = slot->online,
    };

    snprintf(online_changed.module_id, sizeof(online_changed.module_id), "%s", slot->module_id);
    if (handle->config.on_online_changed) {
        ESP_RETURN_ON_ERROR(handle->config.on_online_changed(handle->config.user_context, &online_changed),
                            TAG,
                            "on_online_changed failed");
    }
    return ESP_OK;
}

esp_err_t app_module_runtime_init(const app_module_runtime_config_t *config,
                                  app_module_runtime_handle_t *out_handle)
{
    app_module_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc module runtime failed");

    if (config) {
        handle->config = *config;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->started = true;
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->started = false;
    return ESP_OK;
}

esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_module_runtime_on_rx_frame(app_module_runtime_handle_t handle,
                                         const app_module_runtime_rx_frame_t *frame)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_FALSE(frame && frame->source_tag[0] != '\0', ESP_ERR_INVALID_ARG, TAG, "invalid frame");

    slot = app_module_runtime_find_slot(handle, frame->source_tag, true);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    slot->online = true;
    slot->status = APP_MODULE_RUNTIME_STATUS_ONLINE;
    snprintf(slot->revision, sizeof(slot->revision), "frame_len_%u", (unsigned)frame->len);

    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_info(handle, slot), TAG, "emit info failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_online(handle, slot), TAG, "emit online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_INFO_UPDATED, slot),
                        TAG,
                        "emit module event failed");
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_on_timeout(app_module_runtime_handle_t handle, const char *module_id)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_FALSE(module_id, ESP_ERR_INVALID_ARG, TAG, "module_id is NULL");

    slot = app_module_runtime_find_slot(handle, module_id, false);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NOT_FOUND, TAG, "module not found");

    slot->status = APP_MODULE_RUNTIME_STATUS_FAULT;
    slot->online = false;
    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_TIMEOUT, slot),
                        TAG,
                        "emit timeout event failed");
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_on_port_detected(app_module_runtime_handle_t handle, const char *module_id)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_FALSE(module_id, ESP_ERR_INVALID_ARG, TAG, "module_id is NULL");

    slot = app_module_runtime_find_slot(handle, module_id, true);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    slot->online = true;
    slot->status = APP_MODULE_RUNTIME_STATUS_DETECTED;
    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_online(handle, slot), TAG, "emit online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_PORT_DETECTED, slot),
                        TAG,
                        "emit detected event failed");
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_on_port_lost(app_module_runtime_handle_t handle, const char *module_id)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_FALSE(module_id, ESP_ERR_INVALID_ARG, TAG, "module_id is NULL");

    slot = app_module_runtime_find_slot(handle, module_id, false);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NOT_FOUND, TAG, "module not found");

    slot->online = false;
    slot->status = APP_MODULE_RUNTIME_STATUS_OFFLINE;
    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_online(handle, slot), TAG, "emit online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_PORT_LOST, slot),
                        TAG,
                        "emit lost event failed");
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_on_manual_rescan(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_ERROR(app_module_runtime_emit_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_MANUAL_RESCAN, NULL),
                        TAG,
                        "emit rescan event failed");
    return app_module_runtime_emit_snapshot(handle);
}

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}
