#include "driver_usb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_msc_ota.h"
#include "esp_msc_host.h"

static const char *TAG = "driver_usb_msc_ota";

esp_err_t driver_usb_msc_ota_run(const char *ota_bin_path)
{
    ESP_RETURN_ON_FALSE(ota_bin_path, ESP_ERR_INVALID_ARG, TAG, "ota_bin_path is NULL");

    esp_msc_host_config_t msc_host_config = {
        .base_path = "/usb",
        .host_driver_config = DEFAULT_MSC_HOST_DRIVER_CONFIG(),
        .vfs_fat_mount_config = DEFAULT_ESP_VFS_FAT_MOUNT_CONFIG(),
        .host_config = DEFAULT_USB_HOST_CONFIG(),
    };
    esp_msc_host_handle_t host_handle = NULL;
    ESP_RETURN_ON_ERROR(esp_msc_host_install(&msc_host_config, &host_handle), TAG, "msc host install failed");

    esp_msc_ota_config_t cfg = {
        .ota_bin_path = ota_bin_path,
        .wait_msc_connect = pdMS_TO_TICKS(5000),
    };
    esp_err_t ret = esp_msc_ota(&cfg);

    esp_msc_host_uninstall(host_handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "USB MSC OTA success, please call esp_restart() if needed");
    } else {
        ESP_LOGE(TAG, "USB MSC OTA failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t driver_usb_msc_ota_begin(const char *ota_bin_path,
                                   driver_usb_msc_ota_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(ota_bin_path && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_handle = NULL;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_msc_ota_perform(driver_usb_msc_ota_handle_t handle)
{
    (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_msc_ota_end(driver_usb_msc_ota_handle_t handle)
{
    (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
}
