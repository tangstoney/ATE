#include "app_module_runtime.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "system_module.h"

struct app_module_runtime {
    app_module_runtime_snapshot_t snapshot;
    bool started;
    esp_event_handler_instance_t event_instance;
    uint8_t last_command_id;
    bool last_command_success;
};

static const char *TAG = "app_module_runtime";
// Current SYSTEM_MODULE_EVENT payloads don't carry module_id, so this runtime
// aggregates into a single primary module slot until system events become
// multi-instance aware.
static const char *APP_MODULE_RUNTIME_DEFAULT_ID = "module_0";
static const char *APP_MODULE_RUNTIME_DEFAULT_TYPE = "tested_module";
static const char *APP_MODULE_RUNTIME_DEFAULT_NAME = "Test Module";
static const char *APP_MODULE_RUNTIME_DEFAULT_REVISION = "unknown";

ESP_EVENT_DEFINE_BASE(APP_MODULE_RUNTIME_EVENT);

static esp_err_t app_module_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_MODULE_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          portMAX_DELAY);
}

static app_module_runtime_status_t app_module_runtime_map_system_state(uint8_t state)
{
    switch (state) {
    case 1:
        return APP_MODULE_RUNTIME_STATUS_OFFLINE;
    case 2:
        return APP_MODULE_RUNTIME_STATUS_ONLINE;
    case 3:
        return APP_MODULE_RUNTIME_STATUS_FAULT;
    case 0:
    default:
        return APP_MODULE_RUNTIME_STATUS_UNKNOWN;
    }
}

static bool app_module_runtime_basic_info_equal(const app_module_runtime_basic_info_t *lhs,
                                                const app_module_runtime_basic_info_t *rhs)
{
    return lhs->online == rhs->online &&
           lhs->status == rhs->status &&
           lhs->error_code == rhs->error_code &&
           strncmp(lhs->module_id, rhs->module_id, sizeof(lhs->module_id)) == 0 &&
           strncmp(lhs->module_type, rhs->module_type, sizeof(lhs->module_type)) == 0 &&
           strncmp(lhs->display_name, rhs->display_name, sizeof(lhs->display_name)) == 0 &&
           strncmp(lhs->revision, rhs->revision, sizeof(lhs->revision)) == 0;
}

static app_module_runtime_basic_info_t *app_module_runtime_find_slot(app_module_runtime_handle_t handle,
                                                                     const char *module_id)
{
    size_t i = 0;

    for (i = 0; i < handle->snapshot.module_count; ++i) {
        if (strncmp(handle->snapshot.modules[i].module_id,
                    module_id,
                    sizeof(handle->snapshot.modules[i].module_id)) == 0) {
            return &handle->snapshot.modules[i];
        }
    }

    return NULL;
}

static app_module_runtime_basic_info_t *app_module_runtime_ensure_primary_slot(app_module_runtime_handle_t handle)
{
    app_module_runtime_basic_info_t *slot = app_module_runtime_find_slot(handle,
                                                                         APP_MODULE_RUNTIME_DEFAULT_ID);

    if (slot) {
        return slot;
    }

    if (handle->snapshot.module_count >= APP_MODULE_RUNTIME_MAX_MODULES) {
        return NULL;
    }

    slot = &handle->snapshot.modules[handle->snapshot.module_count++];
    snprintf(slot->module_id, sizeof(slot->module_id), "%s", APP_MODULE_RUNTIME_DEFAULT_ID);
    snprintf(slot->module_type, sizeof(slot->module_type), "%s", APP_MODULE_RUNTIME_DEFAULT_TYPE);
    snprintf(slot->display_name, sizeof(slot->display_name), "%s", APP_MODULE_RUNTIME_DEFAULT_NAME);
    snprintf(slot->revision, sizeof(slot->revision), "%s", APP_MODULE_RUNTIME_DEFAULT_REVISION);
    slot->status = APP_MODULE_RUNTIME_STATUS_UNKNOWN;
    slot->online = false;
    slot->error_code = 0;
    return slot;
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

static esp_err_t app_module_runtime_publish_notify(app_module_runtime_notify_id_t notify_id,
                                                   const app_module_runtime_basic_info_t *slot,
                                                   uint8_t command_id,
                                                   bool command_success)
{
    app_module_runtime_event_t event = {
        .notify_id = notify_id,
        .status = slot ? slot->status : APP_MODULE_RUNTIME_STATUS_UNKNOWN,
        .online = slot ? slot->online : false,
        .error_code = slot ? slot->error_code : 0,
        .command_id = command_id,
        .command_success = command_success,
    };

    if (slot) {
        snprintf(event.module_id, sizeof(event.module_id), "%s", slot->module_id);
    }

    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_module_runtime_publish_info(const app_module_runtime_basic_info_t *slot)
{
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_INFO_UPDATED, slot, sizeof(*slot));
}

static esp_err_t app_module_runtime_publish_online(const app_module_runtime_basic_info_t *slot)
{
    app_module_runtime_online_changed_t online_changed = {
        .online = slot->online,
    };

    snprintf(online_changed.module_id, sizeof(online_changed.module_id), "%s", slot->module_id);
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED,
                                   &online_changed,
                                   sizeof(online_changed));
}

static esp_err_t app_module_runtime_handle_online_event(app_module_runtime_handle_t handle)
{
    app_module_runtime_basic_info_t *slot = NULL;

    slot = app_module_runtime_ensure_primary_slot(handle);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    if (slot->online && slot->status == APP_MODULE_RUNTIME_STATUS_ONLINE) {
        return ESP_OK;
    }

    slot->online = true;
    slot->status = APP_MODULE_RUNTIME_STATUS_ONLINE;
    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(slot), TAG, "post online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_notify(APP_MODULE_RUNTIME_NOTIFY_ONLINE_CHANGED,
                                                          slot,
                                                          0,
                                                          true),
                        TAG,
                        "post notify failed");
    return app_module_runtime_publish_snapshot(handle);
}

static esp_err_t app_module_runtime_handle_offline_event(app_module_runtime_handle_t handle)
{
    app_module_runtime_basic_info_t *slot = NULL;

    slot = app_module_runtime_ensure_primary_slot(handle);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    if (!slot->online && slot->status == APP_MODULE_RUNTIME_STATUS_OFFLINE) {
        return ESP_OK;
    }

    slot->online = false;
    slot->status = APP_MODULE_RUNTIME_STATUS_OFFLINE;
    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(slot), TAG, "post online failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_notify(APP_MODULE_RUNTIME_NOTIFY_ONLINE_CHANGED,
                                                          slot,
                                                          0,
                                                          true),
                        TAG,
                        "post notify failed");
    return app_module_runtime_publish_snapshot(handle);
}

static esp_err_t app_module_runtime_handle_status_updated(app_module_runtime_handle_t handle,
                                                          const system_module_status_event_t *status_event)
{
    app_module_runtime_basic_info_t *slot = NULL;
    app_module_runtime_basic_info_t before = {0};

    ESP_RETURN_ON_FALSE(status_event, ESP_ERR_INVALID_ARG, TAG, "status_event is NULL");

    slot = app_module_runtime_ensure_primary_slot(handle);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    before = *slot;
    slot->status = app_module_runtime_map_system_state(status_event->state);
    slot->error_code = status_event->error_code;

    if (slot->status == APP_MODULE_RUNTIME_STATUS_UNKNOWN &&
        strncmp(slot->revision, APP_MODULE_RUNTIME_DEFAULT_REVISION, sizeof(slot->revision)) != 0) {
        snprintf(slot->revision, sizeof(slot->revision), "%s", APP_MODULE_RUNTIME_DEFAULT_REVISION);
    }

    if (app_module_runtime_basic_info_equal(&before, slot)) {
        return ESP_OK;
    }

    app_module_runtime_refresh_online_count(handle);
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_info(slot), TAG, "post info failed");
    ESP_RETURN_ON_ERROR(app_module_runtime_publish_notify(APP_MODULE_RUNTIME_NOTIFY_INFO_UPDATED,
                                                          slot,
                                                          0,
                                                          true),
                        TAG,
                        "post notify failed");
    return app_module_runtime_publish_snapshot(handle);
}

static esp_err_t app_module_runtime_handle_command_done(app_module_runtime_handle_t handle,
                                                        const system_module_cmd_done_event_t *cmd_event)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_FALSE(cmd_event, ESP_ERR_INVALID_ARG, TAG, "cmd_event is NULL");

    slot = app_module_runtime_ensure_primary_slot(handle);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    handle->last_command_id = cmd_event->cmd;
    handle->last_command_success = cmd_event->success;
    return app_module_runtime_publish_notify(APP_MODULE_RUNTIME_NOTIFY_COMMAND_DONE,
                                             slot,
                                             cmd_event->cmd,
                                             cmd_event->success);
}

static void app_module_runtime_system_module_event_handler(void *handler_arg,
                                                           esp_event_base_t base,
                                                           int32_t event_id,
                                                           void *event_data)
{
    app_module_runtime_handle_t handle = handler_arg;
    esp_err_t err = ESP_OK;

    if (!handle || base != SYSTEM_MODULE_EVENT || !handle->started) {
        return;
    }

    switch (event_id) {
    case SYSTEM_MODULE_EVENT_ONLINE:
        err = app_module_runtime_handle_online_event(handle);
        break;
    case SYSTEM_MODULE_EVENT_OFFLINE:
        err = app_module_runtime_handle_offline_event(handle);
        break;
    case SYSTEM_MODULE_EVENT_STATUS_UPDATED:
        err = app_module_runtime_handle_status_updated(handle, event_data);
        break;
    case SYSTEM_MODULE_EVENT_COMMAND_DONE:
        err = app_module_runtime_handle_command_done(handle, event_data);
        break;
    default:
        err = ESP_OK;
        break;
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "handle system module event %" PRIi32 " failed: %s", event_id, esp_err_to_name(err));
    }
}

esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle)
{
    app_module_runtime_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc module runtime failed");

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    if (handle->started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(SYSTEM_MODULE_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            app_module_runtime_system_module_event_handler,
                                                            handle,
                                                            &handle->event_instance),
                        TAG,
                        "register system module event handler failed");
    handle->started = true;
    return app_module_runtime_publish_snapshot(handle);
}

esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    if (!handle->started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(SYSTEM_MODULE_EVENT,
                                                              ESP_EVENT_ANY_ID,
                                                              handle->event_instance),
                        TAG,
                        "unregister system module event handler failed");
    handle->event_instance = NULL;
    handle->started = false;
    return ESP_OK;
}

esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    if (handle->started && handle->event_instance) {
        (void)esp_event_handler_instance_unregister(SYSTEM_MODULE_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    handle->event_instance);
    }
    free(handle);
    return ESP_OK;
}

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_snapshot, ESP_ERR_INVALID_ARG, TAG, "out_snapshot is NULL");
    *out_snapshot = handle->snapshot;
    return ESP_OK;
}

esp_err_t app_module_runtime_get_module_count(app_module_runtime_handle_t handle,
                                              size_t *out_count)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_count, ESP_ERR_INVALID_ARG, TAG, "out_count is NULL");
    *out_count = handle->snapshot.module_count;
    return ESP_OK;
}

esp_err_t app_module_runtime_get_online_count(app_module_runtime_handle_t handle,
                                              size_t *out_count)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_count, ESP_ERR_INVALID_ARG, TAG, "out_count is NULL");
    *out_count = handle->snapshot.online_count;
    return ESP_OK;
}

esp_err_t app_module_runtime_get_module_by_index(app_module_runtime_handle_t handle,
                                                 size_t index,
                                                 app_module_runtime_basic_info_t *out_info)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_info, ESP_ERR_INVALID_ARG, TAG, "out_info is NULL");
    ESP_RETURN_ON_FALSE(index < handle->snapshot.module_count,
                        ESP_ERR_NOT_FOUND,
                        TAG,
                        "module index out of range");
    *out_info = handle->snapshot.modules[index];
    return ESP_OK;
}

esp_err_t app_module_runtime_get_module_by_id(app_module_runtime_handle_t handle,
                                              const char *module_id,
                                              app_module_runtime_basic_info_t *out_info)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(module_id, ESP_ERR_INVALID_ARG, TAG, "module_id is NULL");
    ESP_RETURN_ON_FALSE(out_info, ESP_ERR_INVALID_ARG, TAG, "out_info is NULL");

    slot = app_module_runtime_find_slot(handle, module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NOT_FOUND, TAG, "module not found");
    *out_info = *slot;
    return ESP_OK;
}

esp_err_t app_module_runtime_is_module_online(app_module_runtime_handle_t handle,
                                              const char *module_id,
                                              bool *out_online)
{
    app_module_runtime_basic_info_t *slot = NULL;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(module_id, ESP_ERR_INVALID_ARG, TAG, "module_id is NULL");
    ESP_RETURN_ON_FALSE(out_online, ESP_ERR_INVALID_ARG, TAG, "out_online is NULL");

    slot = app_module_runtime_find_slot(handle, module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NOT_FOUND, TAG, "module not found");
    *out_online = slot->online;
    return ESP_OK;
}
