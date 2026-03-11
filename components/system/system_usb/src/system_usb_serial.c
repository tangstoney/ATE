#include "system_usb.h"

#include "driver_usb.h"

static driver_usb_cdc_handle_t s_cdc;

esp_err_t system_usb_open_serial(void)
{
    return driver_usb_cdc_open_first(&s_cdc);
}

esp_err_t system_usb_close_serial(void)
{
    return driver_usb_cdc_close(s_cdc);
}
