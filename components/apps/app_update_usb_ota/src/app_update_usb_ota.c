#include "app_update_usb_ota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "system_usb_host.h"

struct app_update_usb_ota {
    app_update_usb_ota_config_t config;
    app_update_usb_ota_progress_t progress;
    app_update_usb_ota_result_t result;
    bool usb_inserted;
};

static const char *TAG = "app_update_usb_ota";
static const char *DEFAULT_OTA_FILE_PATH = "/usb/ota.bin";

static esp_err_t app_update_usb_ota_require_handle(app_update_usb_ota_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    return ESP_OK;
}

static const char *app_update_usb_ota_resolve_file_path(app_update_usb_ota_handle_t handle)
{
    if (handle->progress.active_file_path[0] != '\0') {
        return handle->progress.active_file_path;
    }

    if (handle->config.default_ota_file_path) {
        return handle->config.default_ota_file_path;
    }

    return DEFAULT_OTA_FILE_PATH;
}

static esp_err_t app_update_usb_ota_emit_event(app_update_usb_ota_handle_t handle,
                                               app_update_usb_ota_event_id_t event_id,
                                               esp_err_t result)
{
    app_update_usb_ota_event_t event = {
        .event_id = event_id,
        .status = handle->progress.status,
        .progress_percent = handle->progress.progress_percent,
        .result = result,
    };

    snprintf(event.active_file_path, sizeof(event.active_file_path), "%s", handle->progress.active_file_path);

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event),
                            TAG,
                            "on_event failed");
    }

    return ESP_OK;
}

static esp_err_t app_update_usb_ota_emit_progress(app_update_usb_ota_handle_t handle)
{
    if (handle->config.on_progress) {
        ESP_RETURN_ON_ERROR(handle->config.on_progress(handle->config.user_context, &handle->progress),
                            TAG,
                            "on_progress failed");
    }

    return app_update_usb_ota_emit_event(handle, APP_UPDATE_USB_OTA_EVENT_PROGRESS_UPDATED, ESP_OK);
}

static esp_err_t app_update_usb_ota_emit_result(app_update_usb_ota_handle_t handle,
                                                app_update_usb_ota_event_id_t event_id)
{
    if (handle->config.on_result) {
        ESP_RETURN_ON_ERROR(handle->config.on_result(handle->config.user_context, &handle->result),
                            TAG,
                            "on_result failed");
    }

    return app_update_usb_ota_emit_event(handle, event_id, handle->result.result);
}

esp_err_t app_update_usb_ota_init(const app_update_usb_ota_config_t *config,
                                  app_update_usb_ota_handle_t *out_handle)
{
    app_update_usb_ota_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc usb ota handle failed");

    if (config) {
        handle->config = *config;
    }

    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_IDLE;
    handle->result.result = ESP_OK;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_update_usb_ota_deinit(app_update_usb_ota_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    free(handle);
    return ESP_OK;
}

esp_err_t app_update_usb_ota_on_usb_insert(app_update_usb_ota_handle_t handle, bool inserted)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");

    handle->usb_inserted = inserted;
    handle->progress.status = inserted ? APP_UPDATE_USB_OTA_STATUS_MEDIA_READY : APP_UPDATE_USB_OTA_STATUS_IDLE;
    if (!inserted) {
        handle->progress.progress_percent = 0;
        handle->progress.active_file_path[0] = '\0';
        handle->result = (app_update_usb_ota_result_t){0};
    }

    return app_update_usb_ota_emit_event(handle,
                                         inserted ? APP_UPDATE_USB_OTA_EVENT_USB_INSERTED
                                                  : APP_UPDATE_USB_OTA_EVENT_USB_REMOVED,
                                         ESP_OK);
}

esp_err_t app_update_usb_ota_on_file_detected(app_update_usb_ota_handle_t handle, const char *file_path)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(file_path, ESP_ERR_INVALID_ARG, TAG, "file_path is NULL");

    snprintf(handle->progress.active_file_path, sizeof(handle->progress.active_file_path), "%s", file_path);
    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_FILE_READY;
    return app_update_usb_ota_emit_event(handle, APP_UPDATE_USB_OTA_EVENT_FILE_DETECTED, ESP_OK);
}

esp_err_t app_update_usb_ota_start(app_update_usb_ota_handle_t handle)
{
    const char *file_path = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(handle->usb_inserted || handle->progress.active_file_path[0] != '\0',
                        ESP_ERR_INVALID_STATE,
                        TAG,
                        "usb media or ota file not ready");

    file_path = app_update_usb_ota_resolve_file_path(handle);
    snprintf(handle->progress.active_file_path, sizeof(handle->progress.active_file_path), "%s", file_path);

    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_UPDATING;
    handle->progress.progress_percent = 5;
    ESP_RETURN_ON_ERROR(app_update_usb_ota_emit_event(handle, APP_UPDATE_USB_OTA_EVENT_STARTED, ESP_OK),
                        TAG,
                        "emit start event failed");
    ESP_RETURN_ON_ERROR(app_update_usb_ota_emit_progress(handle), TAG, "emit start progress failed");

    ret = system_usb_host_init();
    if (ret != ESP_OK) {
        handle->progress.status = APP_UPDATE_USB_OTA_STATUS_FAILED;
        handle->result.result = ret;
        handle->result.reboot_required = false;
        snprintf(handle->result.active_file_path, sizeof(handle->result.active_file_path), "%s", file_path);
        return app_update_usb_ota_emit_result(handle, APP_UPDATE_USB_OTA_EVENT_FAILED);
    }

    handle->progress.progress_percent = 35;
    ESP_RETURN_ON_ERROR(app_update_usb_ota_emit_progress(handle), TAG, "emit init progress failed");

    ret = system_usb_host_start_ota(file_path);
    handle->result.result = ret;
    snprintf(handle->result.active_file_path, sizeof(handle->result.active_file_path), "%s", file_path);
    handle->result.reboot_required = (ret == ESP_OK);

    if (ret == ESP_OK) {
        handle->progress.status = APP_UPDATE_USB_OTA_STATUS_SUCCEEDED;
        handle->progress.progress_percent = 100;
        ESP_RETURN_ON_ERROR(app_update_usb_ota_emit_progress(handle), TAG, "emit success progress failed");
        return app_update_usb_ota_emit_result(handle, APP_UPDATE_USB_OTA_EVENT_SUCCEEDED);
    }

    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_FAILED;
    return app_update_usb_ota_emit_result(handle, APP_UPDATE_USB_OTA_EVENT_FAILED);
}

esp_err_t app_update_usb_ota_get_progress(app_update_usb_ota_handle_t handle,
                                          app_update_usb_ota_progress_t *out_progress)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(out_progress, ESP_ERR_INVALID_ARG, TAG, "out_progress is NULL");
    *out_progress = handle->progress;
    return ESP_OK;
}

esp_err_t app_update_usb_ota_get_result(app_update_usb_ota_handle_t handle,
                                        app_update_usb_ota_result_t *out_result)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(out_result, ESP_ERR_INVALID_ARG, TAG, "out_result is NULL");
    *out_result = handle->result;
    return ESP_OK;
}
