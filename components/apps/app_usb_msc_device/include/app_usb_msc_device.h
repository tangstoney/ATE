#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_usb_msc_device *app_usb_msc_device_handle_t;

typedef enum {
    APP_USB_MSC_DEVICE_STATE_DISABLED = 0,
    APP_USB_MSC_DEVICE_STATE_ENABLED,
    APP_USB_MSC_DEVICE_STATE_FAULT,
} app_usb_msc_device_state_t;

typedef struct {
    bool supports_test_context_trigger;
    bool supports_export_request_trigger;
    bool supports_maintenance_trigger;
} app_usb_msc_device_capability_t;

typedef enum {
    APP_USB_MSC_DEVICE_EVENT_ENABLE_REQUESTED = 0,
    APP_USB_MSC_DEVICE_EVENT_DISABLE_REQUESTED,
    APP_USB_MSC_DEVICE_EVENT_ENABLED,
    APP_USB_MSC_DEVICE_EVENT_DISABLED,
} app_usb_msc_device_event_id_t;

typedef struct {
    app_usb_msc_device_event_id_t event_id;
    app_usb_msc_device_state_t state;
} app_usb_msc_device_event_t;

typedef esp_err_t (*app_usb_msc_device_on_state_fn_t)(void *user_context,
                                                      app_usb_msc_device_state_t state);
typedef esp_err_t (*app_usb_msc_device_on_event_fn_t)(void *user_context,
                                                      const app_usb_msc_device_event_t *event);

typedef struct {
    app_usb_msc_device_capability_t capability;
    app_usb_msc_device_on_state_fn_t on_state;
    app_usb_msc_device_on_event_fn_t on_event;
    void *user_context;
} app_usb_msc_device_config_t;

esp_err_t app_usb_msc_device_init(const app_usb_msc_device_config_t *config,
                                  app_usb_msc_device_handle_t *out_handle);
esp_err_t app_usb_msc_device_deinit(app_usb_msc_device_handle_t handle);

esp_err_t app_usb_msc_device_enable(app_usb_msc_device_handle_t handle);
esp_err_t app_usb_msc_device_disable(app_usb_msc_device_handle_t handle);
esp_err_t app_usb_msc_device_get_state(app_usb_msc_device_handle_t handle,
                                       app_usb_msc_device_state_t *out_state);
esp_err_t app_usb_msc_device_get_capability(app_usb_msc_device_handle_t handle,
                                            app_usb_msc_device_capability_t *out_capability);

#ifdef __cplusplus
}
#endif
