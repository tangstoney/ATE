#include "system_usb.h"

#include "driver_usb.h"

static driver_usb_storage_handle_t s_storage;

esp_err_t system_usb_mount_storage(void)
{
    return driver_usb_storage_mount(&s_storage, "/usb");
}

esp_err_t system_usb_unmount_storage(void)
{
    return driver_usb_storage_unmount(s_storage);
}
