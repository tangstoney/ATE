#include "app_update_usb_ota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "app_ui_controller.h"
#include "system_usb_host.h"

struct app_update_usb_ota {
    app_update_usb_ota_progress_t progress;
    app_update_usb_ota_result_t result;
    bool usb_inserted;
    esp_event_handler_instance_t ui_ctrl_start_ota_handler;
};

static const char *TAG = "app_update_usb_ota";
static const char *DEFAULT_OTA_FILE_PATH = "/usb/ota.bin";

ESP_EVENT_DEFINE_BASE(APP_UPDATE_USB_OTA_EVENT);

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

    return DEFAULT_OTA_FILE_PATH;
}

static esp_err_t app_update_usb_ota_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_UPDATE_USB_OTA_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static void app_update_usb_ota_post_ui_progress(app_update_usb_ota_handle_t handle)
{
    app_ui_usb_ota_progress_event_t event = {
        .progress_percent = handle->progress.progress_percent,
    };

    if (esp_event_post(APP_UI_EVENT,
                       APP_UI_EVENT_USB_OTA_PROGRESS,
                       &event,
                       sizeof(event),
                       pdMS_TO_TICKS(100)) != ESP_OK) {
        ESP_LOGW(TAG, "post ui usb ota progress failed");
    }
}

static esp_err_t app_update_usb_ota_publish_event(app_update_usb_ota_handle_t handle,
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
    return app_update_usb_ota_post(APP_UPDATE_USB_OTA_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

static esp_err_t app_update_usb_ota_publish_progress(app_update_usb_ota_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_post(APP_UPDATE_USB_OTA_BUS_EVENT_PROGRESS,
                                                &handle->progress,
                                                sizeof(handle->progress)),
                        TAG,
                        "post progress failed");
    app_update_usb_ota_post_ui_progress(handle);
    return app_update_usb_ota_publish_event(handle, APP_UPDATE_USB_OTA_EVENT_PROGRESS_UPDATED, ESP_OK);
}

static esp_err_t app_update_usb_ota_publish_result(app_update_usb_ota_handle_t handle,
                                                   app_update_usb_ota_event_id_t event_id)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_post(APP_UPDATE_USB_OTA_BUS_EVENT_RESULT,
                                                &handle->result,
                                                sizeof(handle->result)),
                        TAG,
                        "post result failed");
    return app_update_usb_ota_publish_event(handle, event_id, handle->result.result);
}

static esp_err_t app_update_usb_ota_handle_start_request(app_update_usb_ota_handle_t handle,
                                                         const app_ui_controller_start_usb_ota_event_t *event)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");

    if (event && event->update_file_path[0] != '\0') {
        ESP_RETURN_ON_ERROR(app_update_usb_ota_on_file_detected(handle, event->update_file_path),
                            TAG,
                            "record ui-selected usb ota file failed");
    }

    ESP_RETURN_ON_ERROR(app_update_usb_ota_on_usb_insert(handle, true),
                        TAG,
                        "mark usb media ready failed");
    return app_update_usb_ota_start(handle);
}

static void app_update_usb_ota_ui_ctrl_event_handler(void *handler_arg,
                                                     esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     void *event_data)
{
    esp_err_t ret = ESP_OK;

    (void)event_base;

    if (event_id != APP_UI_CTRL_EVENT_START_USB_OTA) {
        return;
    }

    ret = app_update_usb_ota_handle_start_request(handler_arg, event_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "handle start usb ota request failed: %s", esp_err_to_name(ret));
    }
}

esp_err_t app_update_usb_ota_init(app_update_usb_ota_handle_t *out_handle)
{
    app_update_usb_ota_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc usb ota handle failed");

    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_IDLE;
    handle->result.result = ESP_OK;

    ret = esp_event_handler_instance_register(APP_UI_CTRL_EVENT,
                                              APP_UI_CTRL_EVENT_START_USB_OTA,
                                              app_update_usb_ota_ui_ctrl_event_handler,
                                              handle,
                                              &handle->ui_ctrl_start_ota_handler);
    if (ret != ESP_OK) {
        free(handle);
        return ret;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_update_usb_ota_deinit(app_update_usb_ota_handle_t handle)
{
    ESP_RETURN_ON_ERROR(app_update_usb_ota_require_handle(handle), TAG, "invalid handle");
    if (handle->ui_ctrl_start_ota_handler) {
        ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(APP_UI_CTRL_EVENT,
                                                                  APP_UI_CTRL_EVENT_START_USB_OTA,
                                                                  handle->ui_ctrl_start_ota_handler),
                            TAG,
                            "unregister ui start usb ota handler failed");
    }
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

    return app_update_usb_ota_publish_event(handle,
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
    return app_update_usb_ota_publish_event(handle, APP_UPDATE_USB_OTA_EVENT_FILE_DETECTED, ESP_OK);
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
    ESP_RETURN_ON_ERROR(app_update_usb_ota_publish_event(handle, APP_UPDATE_USB_OTA_EVENT_STARTED, ESP_OK),
                        TAG,
                        "post start event failed");
    ESP_RETURN_ON_ERROR(app_update_usb_ota_publish_progress(handle), TAG, "post start progress failed");

    ret = system_usb_host_init();
    if (ret != ESP_OK) {
        handle->progress.status = APP_UPDATE_USB_OTA_STATUS_FAILED;
        handle->result.result = ret;
        handle->result.reboot_required = false;
        snprintf(handle->result.active_file_path, sizeof(handle->result.active_file_path), "%s", file_path);
        return app_update_usb_ota_publish_result(handle, APP_UPDATE_USB_OTA_EVENT_FAILED);
    }

    handle->progress.progress_percent = 35;
    ESP_RETURN_ON_ERROR(app_update_usb_ota_publish_progress(handle), TAG, "post init progress failed");

    ret = system_usb_host_start_ota(file_path);
    handle->result.result = ret;
    snprintf(handle->result.active_file_path, sizeof(handle->result.active_file_path), "%s", file_path);
    handle->result.reboot_required = (ret == ESP_OK);

    if (ret == ESP_OK) {
        handle->progress.status = APP_UPDATE_USB_OTA_STATUS_SUCCEEDED;
        handle->progress.progress_percent = 100;
        ESP_RETURN_ON_ERROR(app_update_usb_ota_publish_progress(handle), TAG, "post success progress failed");
        return app_update_usb_ota_publish_result(handle, APP_UPDATE_USB_OTA_EVENT_SUCCEEDED);
    }

    handle->progress.status = APP_UPDATE_USB_OTA_STATUS_FAILED;
    return app_update_usb_ota_publish_result(handle, APP_UPDATE_USB_OTA_EVENT_FAILED);
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
