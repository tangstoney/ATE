#include "driver_usb.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "usb_host_msc.h"

static const char *TAG = "driver_usb_storage";

typedef struct driver_usb_storage {
    usb_host_msc_handle_t msc_handle;
    char *mount_point;
} driver_usb_storage_t;

esp_err_t driver_usb_storage_mount(driver_usb_storage_handle_t *out_handle, const char *mount_point)
{
    ESP_RETURN_ON_FALSE(out_handle && mount_point, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    driver_usb_storage_t *handle = calloc(1, sizeof(driver_usb_storage_t));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    // TODO: initialize usb_host_msc and mount to VFS.
    handle->mount_point = strdup(mount_point);
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_usb_storage_unmount(driver_usb_storage_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle->mount_point);
    free(handle);
    return ESP_OK;
}
