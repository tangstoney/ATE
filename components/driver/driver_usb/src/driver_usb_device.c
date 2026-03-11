#include "driver_usb_device.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "driver_usb_device";
static bool s_device_installed = false;

esp_err_t driver_usb_device_init(void)
{
    if (s_device_installed) {
        return ESP_OK;
    }

    ESP_LOGW(TAG, "TinyUSB not wired yet, init is stubbed");
    s_device_installed = true;
    return ESP_OK;
}

esp_err_t driver_usb_device_deinit(void)
{
    if (!s_device_installed) {
        return ESP_OK;
    }

    ESP_LOGW(TAG, "TinyUSB not wired yet, deinit is stubbed");
    s_device_installed = false;
    return ESP_OK;
}

esp_err_t driver_usb_device_cdc_enable(void)
{
    ESP_LOGW(TAG, "CDC device enable not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_device_msc_enable_spiflash(void)
{
    ESP_LOGW(TAG, "MSC (SPI flash) device enable not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_device_msc_enable_sdmmc(void)
{
    ESP_LOGW(TAG, "MSC (SDMMC) device enable not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}
