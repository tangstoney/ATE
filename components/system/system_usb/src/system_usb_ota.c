#include "system_usb.h"

#include "esp_log.h"
#include "driver_usb.h"

static const char *TAG = "system_usb_ota";

esp_err_t system_usb_start_ota(void)
{
    ESP_LOGI(TAG, "Start USB OTA");
    return driver_usb_msc_ota_run("/usb/ota.bin");
}
