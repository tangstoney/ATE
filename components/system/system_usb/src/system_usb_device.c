#include "system_usb_device.h"

#include "esp_check.h"
#include "esp_log.h"

#include "driver_usb_device.h"

static const char *TAG = "system_usb_device";
static bool s_device_started = false;

esp_err_t system_usb_device_start(void)
{
    if (s_device_started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(driver_usb_device_init(), TAG, "driver_usb_device_init failed");
    s_device_started = true;
    return ESP_OK;
}

esp_err_t system_usb_device_stop(void)
{
    if (!s_device_started) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(driver_usb_device_deinit(), TAG, "driver_usb_device_deinit failed");
    s_device_started = false;
    return ESP_OK;
}

esp_err_t system_usb_device_start_serial(void)
{
    ESP_RETURN_ON_ERROR(system_usb_device_start(), TAG, "system_usb_device_start failed");
    return driver_usb_device_cdc_enable();
}

esp_err_t system_usb_device_start_storage(void)
{
    ESP_RETURN_ON_ERROR(system_usb_device_start(), TAG, "system_usb_device_start failed");
    return driver_usb_device_msc_enable_spiflash();
}
