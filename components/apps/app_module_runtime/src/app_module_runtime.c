#include "app_module_runtime.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system_module.h"

struct app_module_runtime {
    system_module_handle_t system_module;
    app_module_runtime_snapshot_t snapshot;
    bool started;
    esp_event_handler_instance_t event_instance;
    uint8_t last_command_id;
    bool last_command_success;
};

static const char *TAG = "app_module_runtime";
static const uint8_t APP_MODULE_RUNTIME_CMD_KEY_GPIO_TEST = 0x20U;
static const uint8_t APP_MODULE_RUNTIME_CMD_LEFT_KNOB_TEST = 0x21U;
static const uint8_t APP_MODULE_RUNTIME_CMD_RIGHT_KNOB_TEST = 0x22U;
static const uint8_t APP_MODULE_RUNTIME_CMD_CENTER_KNOB_TEST = 0x23U;
static const size_t APP_MODULE_RUNTIME_KEY_GPIO_TEST_MIN_LEN = 1U;
static const size_t APP_MODULE_RUNTIME_KEY_GPIO_TEST_MAX_LEN = 10U;

ESP_EVENT_DEFINE_BASE(APP_MODULE_RUNTIME_EVENT);

static esp_err_t app_module_runtime_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_MODULE_RUNTIME_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          portMAX_DELAY);
}

static void app_module_runtime_copy_text(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

static app_module_runtime_status_t app_module_runtime_map_system_state(uint8_t state)
{
    switch ((system_module_state_t)state) {
    case SYSTEM_MODULE_STATE_OFFLINE:
        return APP_MODULE_RUNTIME_STATUS_OFFLINE;
    case SYSTEM_MODULE_STATE_ONLINE:
        return APP_MODULE_RUNTIME_STATUS_ONLINE;
    case SYSTEM_MODULE_STATE_ERROR:
        return APP_MODULE_RUNTIME_STATUS_FAULT;
    case SYSTEM_MODULE_STATE_UNKNOWN:
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
           lhs->capability_mask == rhs->capability_mask &&
           strncmp(lhs->module_id, rhs->module_id, sizeof(lhs->module_id)) == 0 &&
           strncmp(lhs->module_name, rhs->module_name, sizeof(lhs->module_name)) == 0 &&
           strncmp(lhs->module_type, rhs->module_type, sizeof(lhs->module_type)) == 0 &&
           strncmp(lhs->revision, rhs->revision, sizeof(lhs->revision)) == 0;
}

static bool app_module_runtime_has_module_id(const char *module_id)
{
    return module_id && module_id[0] != '\0';
}

static bool app_module_runtime_knob_action_valid(app_module_runtime_knob_action_t action)
{
    return action == APP_MODULE_RUNTIME_KNOB_ACTION_NONE ||
           action == APP_MODULE_RUNTIME_KNOB_ACTION_LEFT ||
           action == APP_MODULE_RUNTIME_KNOB_ACTION_RIGHT ||
           action == APP_MODULE_RUNTIME_KNOB_ACTION_PRESS;
}

static app_module_runtime_basic_info_t *app_module_runtime_find_slot(app_module_runtime_handle_t handle,
                                                                     const char *module_id)
{
    size_t i = 0;

    if (!handle || !app_module_runtime_has_module_id(module_id)) {
        return NULL;
    }

    for (i = 0; i < handle->snapshot.module_count; ++i) {
        if (strncmp(handle->snapshot.modules[i].module_id,
                    module_id,
                    sizeof(handle->snapshot.modules[i].module_id)) == 0) {
            return &handle->snapshot.modules[i];
        }
    }

    return NULL;
}

static app_module_runtime_basic_info_t *app_module_runtime_ensure_slot(app_module_runtime_handle_t handle,
                                                                       const char *module_id)
{
    app_module_runtime_basic_info_t *slot = NULL;

    slot = app_module_runtime_find_slot(handle, module_id);
    if (slot) {
        return slot;
    }

    if (!handle || !app_module_runtime_has_module_id(module_id)) {
        return NULL;
    }

    if (handle->snapshot.module_count >= APP_MODULE_RUNTIME_MAX_MODULES) {
        return NULL;
    }

    slot = &handle->snapshot.modules[handle->snapshot.module_count++];
    memset(slot, 0, sizeof(*slot));
    app_module_runtime_copy_text(slot->module_id, sizeof(slot->module_id), module_id);
    slot->status = APP_MODULE_RUNTIME_STATUS_UNKNOWN;
    return slot;
}

static void app_module_runtime_apply_status_identity(app_module_runtime_basic_info_t *slot,
                                                     const system_module_status_event_t *status_event)
{
    if (!slot || !status_event) {
        return;
    }

    app_module_runtime_copy_text(slot->module_id, sizeof(slot->module_id), status_event->module_id);
    app_module_runtime_copy_text(slot->module_name, sizeof(slot->module_name), status_event->module_name);
    app_module_runtime_copy_text(slot->module_type, sizeof(slot->module_type), status_event->module_type);
    app_module_runtime_copy_text(slot->revision, sizeof(slot->revision), status_event->revision);
}

static void app_module_runtime_apply_command_identity(app_module_runtime_basic_info_t *slot,
                                                      const system_module_cmd_done_event_t *cmd_event)
{
    if (!slot || !cmd_event) {
        return;
    }

    app_module_runtime_copy_text(slot->module_id, sizeof(slot->module_id), cmd_event->module_id);
    app_module_runtime_copy_text(slot->module_name, sizeof(slot->module_name), cmd_event->module_name);
    app_module_runtime_copy_text(slot->module_type, sizeof(slot->module_type), cmd_event->module_type);
    app_module_runtime_copy_text(slot->revision, sizeof(slot->revision), cmd_event->revision);
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

static void app_module_runtime_fill_command_result(app_module_runtime_command_result_t *out_result,
                                                   uint8_t command_id,
                                                   const module_status_t *status,
                                                   bool command_success)
{
    if (!out_result) {
        return;
    }

    memset(out_result, 0, sizeof(*out_result));
    out_result->command_id = command_id;
    out_result->command_success = command_success;

    if (!status) {
        return;
    }

    app_module_runtime_copy_text(out_result->module_id,
                                 sizeof(out_result->module_id),
                                 status->module_id);
    out_result->online = status->online;
    out_result->status = app_module_runtime_map_system_state((uint8_t)status->state);
    out_result->error_code = status->error_code;
    out_result->detail_status_code = status->detail_status_code;
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
        app_module_runtime_copy_text(event.module_id, sizeof(event.module_id), slot->module_id);
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

    app_module_runtime_copy_text(online_changed.module_id,
                                 sizeof(online_changed.module_id),
                                 slot->module_id);
    return app_module_runtime_post(APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED,
                                   &online_changed,
                                   sizeof(online_changed));
}

static esp_err_t app_module_runtime_handle_online_event(app_module_runtime_handle_t handle,
                                                        const system_module_status_event_t *status_event)
{
    app_module_runtime_basic_info_t *slot = NULL;
    app_module_runtime_basic_info_t before = {0};

    ESP_RETURN_ON_FALSE(status_event, ESP_ERR_INVALID_ARG, TAG, "status_event is NULL");
    ESP_RETURN_ON_FALSE(app_module_runtime_has_module_id(status_event->module_id),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "module_id is missing");

    slot = app_module_runtime_ensure_slot(handle, status_event->module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    before = *slot;
    app_module_runtime_apply_status_identity(slot, status_event);
    slot->online = true;
    slot->status = app_module_runtime_map_system_state(status_event->state);
    if (slot->status == APP_MODULE_RUNTIME_STATUS_UNKNOWN) {
        slot->status = APP_MODULE_RUNTIME_STATUS_ONLINE;
    }
    slot->error_code = status_event->error_code;

    if (app_module_runtime_basic_info_equal(&before, slot)) {
        return ESP_OK;
    }

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

static esp_err_t app_module_runtime_handle_offline_event(app_module_runtime_handle_t handle,
                                                         const system_module_status_event_t *status_event)
{
    app_module_runtime_basic_info_t *slot = NULL;
    app_module_runtime_basic_info_t before = {0};

    ESP_RETURN_ON_FALSE(status_event, ESP_ERR_INVALID_ARG, TAG, "status_event is NULL");
    ESP_RETURN_ON_FALSE(app_module_runtime_has_module_id(status_event->module_id),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "module_id is missing");

    slot = app_module_runtime_ensure_slot(handle, status_event->module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    before = *slot;
    app_module_runtime_apply_status_identity(slot, status_event);
    slot->online = false;
    slot->status = app_module_runtime_map_system_state(status_event->state);
    if (slot->status == APP_MODULE_RUNTIME_STATUS_UNKNOWN) {
        slot->status = APP_MODULE_RUNTIME_STATUS_OFFLINE;
    }
    slot->error_code = status_event->error_code;

    if (app_module_runtime_basic_info_equal(&before, slot)) {
        return ESP_OK;
    }

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
    ESP_RETURN_ON_FALSE(app_module_runtime_has_module_id(status_event->module_id),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "module_id is missing");

    slot = app_module_runtime_ensure_slot(handle, status_event->module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    before = *slot;
    app_module_runtime_apply_status_identity(slot, status_event);
    slot->online = status_event->online;
    slot->status = app_module_runtime_map_system_state(status_event->state);
    slot->error_code = status_event->error_code;

    if (app_module_runtime_basic_info_equal(&before, slot)) {
        return ESP_OK;
    }

    app_module_runtime_refresh_online_count(handle);
    if (before.online != slot->online) {
        ESP_RETURN_ON_ERROR(app_module_runtime_publish_online(slot), TAG, "post online failed");
        ESP_RETURN_ON_ERROR(app_module_runtime_publish_notify(APP_MODULE_RUNTIME_NOTIFY_ONLINE_CHANGED,
                                                              slot,
                                                              0,
                                                              true),
                            TAG,
                            "post notify failed");
    }
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
    app_module_runtime_basic_info_t before = {0};

    ESP_RETURN_ON_FALSE(cmd_event, ESP_ERR_INVALID_ARG, TAG, "cmd_event is NULL");
    ESP_RETURN_ON_FALSE(app_module_runtime_has_module_id(cmd_event->module_id),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "module_id is missing");

    slot = app_module_runtime_ensure_slot(handle, cmd_event->module_id);
    ESP_RETURN_ON_FALSE(slot, ESP_ERR_NO_MEM, TAG, "no module slot available");

    before = *slot;
    app_module_runtime_apply_command_identity(slot, cmd_event);

    if (!app_module_runtime_basic_info_equal(&before, slot)) {
        ESP_RETURN_ON_ERROR(app_module_runtime_publish_info(slot), TAG, "post info failed");
        ESP_RETURN_ON_ERROR(app_module_runtime_publish_snapshot(handle), TAG, "post snapshot failed");
    }

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
        err = app_module_runtime_handle_online_event(handle, event_data);
        break;
    case SYSTEM_MODULE_EVENT_OFFLINE:
        err = app_module_runtime_handle_offline_event(handle, event_data);
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

static esp_err_t app_module_runtime_send_business_command(app_module_runtime_handle_t handle,
                                                          uint8_t command_id,
                                                          const uint8_t *payload,
                                                          size_t payload_len,
                                                          app_module_runtime_command_result_t *out_result)
{
    module_status_t status = {0};
    esp_err_t send_err = ESP_OK;
    esp_err_t status_err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    if (payload_len > 0U) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }

    send_err = system_module_send_command(handle->system_module, command_id, payload, payload_len);
    status_err = system_module_get_status(handle->system_module, &status);

    if (status_err == ESP_OK) {
        app_module_runtime_fill_command_result(out_result,
                                               command_id,
                                               &status,
                                               send_err == ESP_OK &&
                                               status.detail_status_code == 0U);
    } else {
        app_module_runtime_fill_command_result(out_result, command_id, NULL, false);
    }

    if (send_err != ESP_OK) {
        return send_err;
    }
    return status_err;
}

static const char *app_module_runtime_status_to_string(app_module_runtime_status_t status)
{
    switch (status) {
    case APP_MODULE_RUNTIME_STATUS_OFFLINE:
        return "offline";
    case APP_MODULE_RUNTIME_STATUS_ONLINE:
        return "online";
    case APP_MODULE_RUNTIME_STATUS_FAULT:
        return "fault";
    case APP_MODULE_RUNTIME_STATUS_UNKNOWN:
    default:
        return "unknown";
    }
}

static void app_module_runtime_debug_wait_for_event_delivery(void)
{
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void app_module_runtime_debug_log_command_result(const char *label,
                                                        esp_err_t request_err,
                                                        const app_module_runtime_command_result_t *result)
{
    if (!result) {
        ESP_LOGI(TAG, "[debug] %s: request_err=%s", label, esp_err_to_name(request_err));
        return;
    }

    ESP_LOGI(TAG,
             "[debug] %s result: request_err=%s module_id=%s cmd=0x%02X success=%d online=%d status=%s error=0x%04X detail_status=0x%04X",
             label,
             esp_err_to_name(request_err),
             result->module_id,
             result->command_id,
             result->command_success,
             result->online,
             app_module_runtime_status_to_string(result->status),
             result->error_code,
             result->detail_status_code);
}

esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle)
{
    app_module_runtime_handle_t handle = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc module runtime failed");

    err = system_module_create(&handle->system_module);
    if (err != ESP_OK) {
        free(handle);
        return err;
    }

    err = app_module_runtime_start(handle);
    if (err != ESP_OK) {
        (void)system_module_destroy(handle->system_module);
        free(handle);
        return err;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle)
{
    bool online = false;
    esp_err_t probe_err = ESP_OK;

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

    probe_err = system_module_is_online(handle->system_module, &online);
    if (probe_err != ESP_OK) {
        ESP_LOGW(TAG, "initial system_module probe failed: %s", esp_err_to_name(probe_err));
    } else {
        ESP_LOGI(TAG, "initial system_module online=%d", online);
    }

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

    if (handle->started) {
        (void)app_module_runtime_stop(handle);
    }
    if (handle->system_module) {
        (void)system_module_destroy(handle->system_module);
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

esp_err_t app_module_runtime_request_key_gpio_test(app_module_runtime_handle_t handle,
                                                   const uint8_t *payload,
                                                   size_t payload_len,
                                                   app_module_runtime_command_result_t *out_result)
{
    ESP_RETURN_ON_FALSE(payload_len >= APP_MODULE_RUNTIME_KEY_GPIO_TEST_MIN_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "key gpio payload too short");
    ESP_RETURN_ON_FALSE(payload_len <= APP_MODULE_RUNTIME_KEY_GPIO_TEST_MAX_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "key gpio payload too large");
    return app_module_runtime_send_business_command(handle,
                                                    APP_MODULE_RUNTIME_CMD_KEY_GPIO_TEST,
                                                    payload,
                                                    payload_len,
                                                    out_result);
}

esp_err_t app_module_runtime_request_left_knob_test(app_module_runtime_handle_t handle,
                                                    app_module_runtime_knob_action_t action,
                                                    app_module_runtime_command_result_t *out_result)
{
    uint8_t payload = (uint8_t)action;

    ESP_RETURN_ON_FALSE(app_module_runtime_knob_action_valid(action),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "invalid left knob action");
    return app_module_runtime_send_business_command(handle,
                                                    APP_MODULE_RUNTIME_CMD_LEFT_KNOB_TEST,
                                                    &payload,
                                                    sizeof(payload),
                                                    out_result);
}

esp_err_t app_module_runtime_request_right_knob_test(app_module_runtime_handle_t handle,
                                                     app_module_runtime_knob_action_t action,
                                                     app_module_runtime_command_result_t *out_result)
{
    uint8_t payload = (uint8_t)action;

    ESP_RETURN_ON_FALSE(app_module_runtime_knob_action_valid(action),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "invalid right knob action");
    return app_module_runtime_send_business_command(handle,
                                                    APP_MODULE_RUNTIME_CMD_RIGHT_KNOB_TEST,
                                                    &payload,
                                                    sizeof(payload),
                                                    out_result);
}

esp_err_t app_module_runtime_request_center_knob_test(app_module_runtime_handle_t handle,
                                                      app_module_runtime_knob_action_t action,
                                                      app_module_runtime_command_result_t *out_result)
{
    uint8_t payload = (uint8_t)action;

    ESP_RETURN_ON_FALSE(app_module_runtime_knob_action_valid(action),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "invalid center knob action");
    return app_module_runtime_send_business_command(handle,
                                                    APP_MODULE_RUNTIME_CMD_CENTER_KNOB_TEST,
                                                    &payload,
                                                    sizeof(payload),
                                                    out_result);
}

/*
 * Protocol smoke test helpers are intentionally kept inside app_module_runtime
 * as dormant debug tools. They are not part of the normal app startup flow;
 * callers should invoke them only when a real device needs protocol re-checks.
 */
esp_err_t app_module_runtime_debug_log_summary(app_module_runtime_handle_t handle,
                                               const char *stage)
{
    const char *resolved_stage = (stage && stage[0] != '\0') ? stage : "snapshot";
    app_module_runtime_basic_info_t info = {0};
    size_t module_count = 0;
    size_t online_count = 0;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    err = app_module_runtime_get_module_count(handle, &module_count);
    ESP_RETURN_ON_ERROR(err, TAG, "get_module_count failed");

    err = app_module_runtime_get_online_count(handle, &online_count);
    ESP_RETURN_ON_ERROR(err, TAG, "get_online_count failed");

    ESP_LOGI(TAG,
             "[debug] %s summary: module_count=%u online_count=%u",
             resolved_stage,
             (unsigned int)module_count,
             (unsigned int)online_count);

    if (module_count == 0U) {
        return ESP_OK;
    }

    err = app_module_runtime_get_module_by_index(handle, 0U, &info);
    ESP_RETURN_ON_ERROR(err, TAG, "get_module_by_index(0) failed");

    ESP_LOGI(TAG,
             "[debug] %s slot[0]: id=%s name=%s type=%s rev=%s online=%d status=%s error=0x%04X",
             resolved_stage,
             info.module_id,
             info.module_name,
             info.module_type,
             info.revision,
             info.online,
             app_module_runtime_status_to_string(info.status),
             info.error_code);
    return ESP_OK;
}

esp_err_t app_module_runtime_debug_run_protocol_smoke_test(app_module_runtime_handle_t handle)
{
    static const uint8_t key_gpio_payload[] = {0x12, 0x34, 0x56};

    app_module_runtime_command_result_t result = {0};
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    ESP_LOGI(TAG, "[debug] protocol smoke test begin");

    err = app_module_runtime_debug_log_summary(handle, "baseline");
    ESP_RETURN_ON_ERROR(err, TAG, "debug log baseline failed");

    err = app_module_runtime_request_key_gpio_test(handle,
                                                   key_gpio_payload,
                                                   sizeof(key_gpio_payload),
                                                   &result);
    app_module_runtime_debug_log_command_result("key_gpio_test", err, &result);
    app_module_runtime_debug_wait_for_event_delivery();
    (void)app_module_runtime_debug_log_summary(handle, "after key_gpio_test");

    err = app_module_runtime_request_left_knob_test(handle,
                                                    APP_MODULE_RUNTIME_KNOB_ACTION_LEFT,
                                                    &result);
    app_module_runtime_debug_log_command_result("left_knob_test", err, &result);
    app_module_runtime_debug_wait_for_event_delivery();
    (void)app_module_runtime_debug_log_summary(handle, "after left_knob_test");

    err = app_module_runtime_request_right_knob_test(handle,
                                                     APP_MODULE_RUNTIME_KNOB_ACTION_RIGHT,
                                                     &result);
    app_module_runtime_debug_log_command_result("right_knob_test", err, &result);
    app_module_runtime_debug_wait_for_event_delivery();
    (void)app_module_runtime_debug_log_summary(handle, "after right_knob_test");

    err = app_module_runtime_request_center_knob_test(handle,
                                                      APP_MODULE_RUNTIME_KNOB_ACTION_PRESS,
                                                      &result);
    app_module_runtime_debug_log_command_result("center_knob_test", err, &result);
    app_module_runtime_debug_wait_for_event_delivery();
    (void)app_module_runtime_debug_log_summary(handle, "after center_knob_test");

    ESP_LOGI(TAG, "[debug] protocol smoke test end");
    return ESP_OK;
}
