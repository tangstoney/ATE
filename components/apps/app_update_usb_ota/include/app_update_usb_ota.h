#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_UPDATE_USB_OTA_FILE_PATH_MAX_LEN 128

typedef struct app_update_usb_ota *app_update_usb_ota_handle_t;

ESP_EVENT_DECLARE_BASE(APP_UPDATE_USB_OTA_EVENT);

typedef enum {
    APP_UPDATE_USB_OTA_STATUS_IDLE = 0,
    APP_UPDATE_USB_OTA_STATUS_MEDIA_READY,
    APP_UPDATE_USB_OTA_STATUS_FILE_READY,
    APP_UPDATE_USB_OTA_STATUS_UPDATING,
    APP_UPDATE_USB_OTA_STATUS_SUCCEEDED,
    APP_UPDATE_USB_OTA_STATUS_FAILED,
} app_update_usb_ota_status_t;

typedef enum {
    APP_UPDATE_USB_OTA_EVENT_USB_INSERTED = 0,
    APP_UPDATE_USB_OTA_EVENT_USB_REMOVED,
    APP_UPDATE_USB_OTA_EVENT_FILE_DETECTED,
    APP_UPDATE_USB_OTA_EVENT_STARTED,
    APP_UPDATE_USB_OTA_EVENT_PROGRESS_UPDATED,
    APP_UPDATE_USB_OTA_EVENT_SUCCEEDED,
    APP_UPDATE_USB_OTA_EVENT_FAILED,
} app_update_usb_ota_event_id_t;

typedef enum {
    APP_UPDATE_USB_OTA_BUS_EVENT_NOTIFY = 0,
    APP_UPDATE_USB_OTA_BUS_EVENT_PROGRESS,
    APP_UPDATE_USB_OTA_BUS_EVENT_RESULT,
} app_update_usb_ota_bus_event_id_t;

typedef struct {
    app_update_usb_ota_status_t status;
    uint8_t progress_percent;
    char active_file_path[APP_UPDATE_USB_OTA_FILE_PATH_MAX_LEN];
} app_update_usb_ota_progress_t;

typedef struct {
    esp_err_t result;
    bool reboot_required;
    char active_file_path[APP_UPDATE_USB_OTA_FILE_PATH_MAX_LEN];
} app_update_usb_ota_result_t;

typedef struct {
    app_update_usb_ota_event_id_t event_id;
    app_update_usb_ota_status_t status;
    uint8_t progress_percent;
    esp_err_t result;
    char active_file_path[APP_UPDATE_USB_OTA_FILE_PATH_MAX_LEN];
} app_update_usb_ota_event_t;

esp_err_t app_update_usb_ota_init(app_update_usb_ota_handle_t *out_handle);
esp_err_t app_update_usb_ota_deinit(app_update_usb_ota_handle_t handle);

esp_err_t app_update_usb_ota_on_usb_insert(app_update_usb_ota_handle_t handle, bool inserted);
esp_err_t app_update_usb_ota_on_file_detected(app_update_usb_ota_handle_t handle, const char *file_path);
esp_err_t app_update_usb_ota_start(app_update_usb_ota_handle_t handle);

esp_err_t app_update_usb_ota_get_progress(app_update_usb_ota_handle_t handle,
                                          app_update_usb_ota_progress_t *out_progress);
esp_err_t app_update_usb_ota_get_result(app_update_usb_ota_handle_t handle,
                                        app_update_usb_ota_result_t *out_result);

#ifdef __cplusplus
}
#endif
