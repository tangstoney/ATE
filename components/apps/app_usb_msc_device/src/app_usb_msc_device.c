#include "app_usb_msc_device.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_event.h"

struct app_usb_msc_device {
    app_usb_msc_device_capability_t capability;
    app_usb_msc_device_state_t state;
};

static const char *TAG = "app_usb_msc_dev";

ESP_EVENT_DEFINE_BASE(APP_USB_MSC_DEVICE_EVENT);

static const app_usb_msc_device_capability_t s_default_capability = {
    .supports_test_context_trigger = true,
    .supports_export_request_trigger = true,
    .supports_maintenance_trigger = true,
};

static esp_err_t app_usb_msc_device_post(int32_t event_id, const void *event_data, size_t event_data_size)
{
    return esp_event_post(APP_USB_MSC_DEVICE_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_usb_msc_device_publish(app_usb_msc_device_handle_t handle,
                                            app_usb_msc_device_event_id_t event_id)
{
    app_usb_msc_device_event_t event = {
        .event_id = event_id,
        .state = handle->state,
    };
    ESP_RETURN_ON_ERROR(app_usb_msc_device_post(APP_USB_MSC_DEVICE_BUS_EVENT_STATE,
                                                &handle->state,
                                                sizeof(handle->state)),
                        TAG,
                        "post state failed");
    return app_usb_msc_device_post(APP_USB_MSC_DEVICE_BUS_EVENT_NOTIFY, &event, sizeof(event));
}

esp_err_t app_usb_msc_device_init(app_usb_msc_device_handle_t *out_handle)
{
    app_usb_msc_device_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc usb msc device failed");

    handle->capability = s_default_capability;
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
    ESP_RETURN_ON_ERROR(app_usb_msc_device_publish(handle, APP_USB_MSC_DEVICE_EVENT_ENABLE_REQUESTED),
                        TAG,
                        "post enable requested failed");
    return app_usb_msc_device_publish(handle, APP_USB_MSC_DEVICE_EVENT_ENABLED);
}

esp_err_t app_usb_msc_device_disable(app_usb_msc_device_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    handle->state = APP_USB_MSC_DEVICE_STATE_DISABLED;
    ESP_RETURN_ON_ERROR(app_usb_msc_device_publish(handle, APP_USB_MSC_DEVICE_EVENT_DISABLE_REQUESTED),
                        TAG,
                        "post disable requested failed");
    return app_usb_msc_device_publish(handle, APP_USB_MSC_DEVICE_EVENT_DISABLED);
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
    *out_capability = handle->capability;
    return ESP_OK;
}
