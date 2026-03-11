#include "app_usb.h"

#include "system_usb_host.h"

esp_err_t app_usb_start(void)
{
    system_usb_host_init();
    system_usb_host_start_ota("/usb/ota.bin");
    system_usb_host_open_serial();
    return ESP_OK;
}
