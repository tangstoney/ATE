#include "system_usb_host.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_msc_ota.h"
#include "esp_msc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver_usb_host.h"

static const char *TAG = "system_usb_host";
static bool s_host_inited = false;

esp_err_t system_usb_host_init(void)
{
    if (s_host_inited) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(driver_usb_host_init(), TAG, "driver_usb_host_init failed");
    s_host_inited = true;
    return ESP_OK;
}

esp_err_t system_usb_host_deinit(void)
{
    if (!s_host_inited) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(driver_usb_host_deinit(), TAG, "driver_usb_host_deinit failed");
    s_host_inited = false;
    return ESP_OK;
}

esp_err_t system_usb_host_mount_storage(void)
{
    ESP_RETURN_ON_ERROR(system_usb_host_init(), TAG, "system_usb_host_init failed");
    return driver_usb_host_msc_mount("/usb");
}

esp_err_t system_usb_host_unmount_storage(void)
{
    return driver_usb_host_msc_unmount();
}

esp_err_t system_usb_host_start_ota(const char *ota_bin_path)
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
        ESP_LOGI(TAG, "USB MSC OTA success, please call esp_restart()");
    } else {
        ESP_LOGE(TAG, "USB MSC OTA failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t system_usb_host_open_serial(void)
{
    ESP_RETURN_ON_ERROR(system_usb_host_init(), TAG, "system_usb_host_init failed");
    return driver_usb_host_cdc_open();
}

esp_err_t system_usb_host_close_serial(void)
{
    return driver_usb_host_cdc_close();
}
