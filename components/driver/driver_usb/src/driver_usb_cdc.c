#include "driver_usb.h"

#include <stdlib.h>

#include "esp_check.h"
#include "esp_log.h"
#include "usb_host_cdc_acm.h"

static const char *TAG = "driver_usb_cdc";

typedef struct driver_usb_cdc {
    usb_host_cdc_acm_dev_handle_t cdc_handle;
} driver_usb_cdc_t;

esp_err_t driver_usb_cdc_open_first(driver_usb_cdc_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    driver_usb_cdc_t *handle = calloc(1, sizeof(driver_usb_cdc_t));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    // TODO: open first CDC ACM device.
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_usb_cdc_close(driver_usb_cdc_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}
