#include "app_module_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_module_runtime {
    app_module_runtime_snapshot_t snapshot;
    bool started;
};

static const char *TAG = "app_module_runtime";

ESP_EVENT_DEFINE_BASE(APP_MODULE_RUNTIME_EVENT);

static esp_err_t app_module_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_MODULE_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

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

static esp_err_t app_module_runtime_publish_snapshot(app_module_runtime_handle_t handle)
{
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_SNAPSHOT,
                                   &handle->snapshot,
                                   sizeof(handle->snapshot));
}

static esp_err_t app_module_runtime_publish_event(app_module_runtime_handle_t handle,
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

    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_module_runtime_publish_info(app_module_runtime_handle_t handle,
                                                 const app_module_runtime_basic_info_t *slot)
{
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_INFO_UPDATED, slot, sizeof(*slot));
}

static esp_err_t app_module_runtime_publish_online(app_module_runtime_handle_t handle,
                                                   const app_module_runtime_basic_info_t *slot)
{
    app_module_runtime_online_changed_t online_changed = {
        .online = slot->online,
    };

    snprintf(online_changed.module_id, sizeof(online_changed.module_id), "%s", slot->module_id);
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED,
                                   &online_changed,
                                   sizeof(online_changed));
}

esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle)
{
    app_module_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc module runtime failed");

    *out_handle = handle;
    // codex todo 一键初始化system_module

    return ESP_OK;
}

esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->started = true;
    return app_module_runtime_publish_snapshot(handle);
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
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_info(handle, slot), TAG, "post info failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(handle, slot), TAG, "post online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_INFO_UPDATED, slot),
                        TAG,
                        "post module event failed");
    return app_module_runtime_publish_snapshot(handle);
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
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_TIMEOUT, slot),
                        TAG,
                        "post timeout event failed");
    return app_module_runtime_publish_snapshot(handle);
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
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(handle, slot), TAG, "post online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_PORT_DETECTED, slot),
                        TAG,
                        "post detected event failed");
    return app_module_runtime_publish_snapshot(handle);
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
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(handle, slot), TAG, "post online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_PORT_LOST, slot),
                        TAG,
                        "post lost event failed");
    return app_module_runtime_publish_snapshot(handle);
}

esp_err_t app_module_runtime_on_manual_rescan(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_module_runtime_require_started(handle), TAG, "module runtime not ready");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_event(handle, APP_MODULE_RUNTIME_EVENT_MODULE_MANUAL_RESCAN, NULL),
                        TAG,
                        "post rescan event failed");
    return app_module_runtime_publish_snapshot(handle);
}

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}
