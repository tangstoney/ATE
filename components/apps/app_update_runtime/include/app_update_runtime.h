#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#include "app_update_network_ota.h"
#include "app_update_usb_ota.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_update_runtime *app_update_runtime_handle_t;

ESP_EVENT_DECLARE_BASE(APP_UPDATE_RUNTIME_EVENT);

typedef enum {
    APP_UPDATE_RUNTIME_SOURCE_NONE = 0,
    APP_UPDATE_RUNTIME_SOURCE_USB_OTA,
    APP_UPDATE_RUNTIME_SOURCE_NETWORK_OTA,
} app_update_runtime_source_t;

typedef enum {
    APP_UPDATE_RUNTIME_EVENT_SOURCE_CHANGED = 0,
    APP_UPDATE_RUNTIME_EVENT_PROGRESS_UPDATED,
    APP_UPDATE_RUNTIME_EVENT_RESULT_UPDATED,
} app_update_runtime_event_id_t;

typedef enum {
    APP_UPDATE_RUNTIME_BUS_EVENT_NOTIFY = 0,
    APP_UPDATE_RUNTIME_BUS_EVENT_SNAPSHOT,
} app_update_runtime_bus_event_id_t;

typedef struct {
    app_update_runtime_source_t active_source;
    uint8_t progress_percent;
    esp_err_t last_result;
} app_update_runtime_snapshot_t;

typedef struct {
    app_update_runtime_event_id_t event_id;
    app_update_runtime_snapshot_t snapshot;
} app_update_runtime_event_t;

esp_err_t app_update_runtime_init(app_update_runtime_handle_t *out_handle);
esp_err_t app_update_runtime_deinit(app_update_runtime_handle_t handle);

esp_err_t app_update_runtime_on_usb_ota_event(app_update_runtime_handle_t handle,
                                              const app_update_usb_ota_event_t *event);
esp_err_t app_update_runtime_on_network_ota_event(app_update_runtime_handle_t handle,
                                                  const app_update_network_ota_event_t *event);
esp_err_t app_update_runtime_get_snapshot(app_update_runtime_handle_t handle,
                                          app_update_runtime_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif
