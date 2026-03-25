#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_update_network_ota *app_update_network_ota_handle_t;

ESP_EVENT_DECLARE_BASE(APP_UPDATE_NETWORK_OTA_EVENT);

typedef enum {
    APP_UPDATE_NETWORK_OTA_STATUS_IDLE = 0,
    APP_UPDATE_NETWORK_OTA_STATUS_CHECKING,
    APP_UPDATE_NETWORK_OTA_STATUS_DOWNLOADING,
    APP_UPDATE_NETWORK_OTA_STATUS_APPLYING,
    APP_UPDATE_NETWORK_OTA_STATUS_SUCCEEDED,
    APP_UPDATE_NETWORK_OTA_STATUS_FAILED,
} app_update_network_ota_status_t;

typedef enum {
    APP_UPDATE_NETWORK_OTA_EVENT_STARTED = 0,
    APP_UPDATE_NETWORK_OTA_EVENT_PROGRESS_UPDATED,
    APP_UPDATE_NETWORK_OTA_EVENT_STOPPED,
    APP_UPDATE_NETWORK_OTA_EVENT_SUCCEEDED,
    APP_UPDATE_NETWORK_OTA_EVENT_FAILED,
} app_update_network_ota_event_id_t;

typedef enum {
    APP_UPDATE_NETWORK_OTA_BUS_EVENT_NOTIFY = 0,
    APP_UPDATE_NETWORK_OTA_BUS_EVENT_PROGRESS,
    APP_UPDATE_NETWORK_OTA_BUS_EVENT_RESULT,
} app_update_network_ota_bus_event_id_t;

typedef struct {
    app_update_network_ota_status_t status;
    uint8_t progress_percent;
} app_update_network_ota_progress_t;

typedef struct {
    esp_err_t result;
    bool reboot_required;
} app_update_network_ota_result_t;

typedef struct {
    app_update_network_ota_event_id_t event_id;
    app_update_network_ota_status_t status;
    uint8_t progress_percent;
    esp_err_t result;
} app_update_network_ota_event_t;

esp_err_t app_update_network_ota_init(app_update_network_ota_handle_t *out_handle);
esp_err_t app_update_network_ota_deinit(app_update_network_ota_handle_t handle);

esp_err_t app_update_network_ota_start(app_update_network_ota_handle_t handle);
esp_err_t app_update_network_ota_stop(app_update_network_ota_handle_t handle);
esp_err_t app_update_network_ota_get_progress(app_update_network_ota_handle_t handle,
                                              app_update_network_ota_progress_t *out_progress);
esp_err_t app_update_network_ota_get_result(app_update_network_ota_handle_t handle,
                                            app_update_network_ota_result_t *out_result);

#ifdef __cplusplus
}
#endif
