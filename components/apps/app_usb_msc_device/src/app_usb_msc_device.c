#include "app_usb_msc_device.h"

#include <stdlib.h>

#include "esp_check.h"

struct app_usb_msc_device {
    app_usb_msc_device_config_t config;
    app_usb_msc_device_state_t state;
};

static const char *TAG = "app_usb_msc_dev";

static esp_err_t app_usb_msc_device_emit(app_usb_msc_device_handle_t handle,
                                         app_usb_msc_device_event_id_t event_id)
{
    app_usb_msc_device_event_t event = {
        .event_id = event_id,
        .state = handle->state,
    };

    if (handle->config.on_state) {
        ESP_RETURN_ON_ERROR(handle->config.on_state(handle->config.user_context, handle->state),
                            TAG,
                            "on_state failed");
    }

    if (handle->config.on_event) {
        ESP_RETURN_ON_ERROR(handle->config.on_event(handle->config.user_context, &event),
                            TAG,
                            "on_event failed");
    }

    return ESP_OK;
}

esp_err_t app_usb_msc_device_init(const app_usb_msc_device_config_t *config,
                                  app_usb_msc_device_handle_t *out_handle)
{
    app_usb_msc_device_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc usb msc device failed");

    if (config) {
        handle->config = *config;
    }
    handle->state = APP_USB_MSC_DEVICE_STATE_DISABLED;
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t app_usb_msc_device_deinit(app_usb_msc_device_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    free(handle);
    return ESP_OK;
}

esp_err_t app_usb_msc_device_enable(app_usb_msc_device_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->state = APP_USB_MSC_DEVICE_STATE_ENABLED;
    ESP_RETURN_ON_ERROR(app_usb_msc_device_emit(handle, APP_USB_MSC_DEVICE_EVENT_ENABLE_REQUESTED),
                        TAG,
                        "emit enable requested failed");
    return app_usb_msc_device_emit(handle, APP_USB_MSC_DEVICE_EVENT_ENABLED);
}

esp_err_t app_usb_msc_device_disable(app_usb_msc_device_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->state = APP_USB_MSC_DEVICE_STATE_DISABLED;
    ESP_RETURN_ON_ERROR(app_usb_msc_device_emit(handle, APP_USB_MSC_DEVICE_EVENT_DISABLE_REQUESTED),
                        TAG,
                        "emit disable requested failed");
    return app_usb_msc_device_emit(handle, APP_USB_MSC_DEVICE_EVENT_DISABLED);
}

esp_err_t app_usb_msc_device_get_state(app_usb_msc_device_handle_t handle,
                                       app_usb_msc_device_state_t *out_state)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_state, ESP_ERR_INVALID_ARG, TAG, "out_state is NULL");
    *out_state = handle->state;
    return ESP_OK;
}

esp_err_t app_usb_msc_device_get_capability(app_usb_msc_device_handle_t handle,
                                            app_usb_msc_device_capability_t *out_capability)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(out_capability, ESP_ERR_INVALID_ARG, TAG, "out_capability is NULL");
    *out_capability = handle->config.capability;
    return ESP_OK;
}
