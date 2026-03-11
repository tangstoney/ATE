#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB Host service.
 *
 * Internally calls driver_usb_host_init().
 * Manages USB Host lifecycle.
 */
esp_err_t system_usb_host_init(void);

/**
 * @brief Deinitialize USB Host service.
 *
 * Ensures all class drivers are stopped before teardown.
 */
esp_err_t system_usb_host_deinit(void);

/**
 * @brief Mount USB flash drive to VFS.
 *
 * Calls driver_usb_host_msc_mount("/usb").
 * After success, standard C file APIs (fopen, fread, etc.) are available.
 */
esp_err_t system_usb_host_mount_storage(void);

/**
 * @brief Unmount USB flash drive from VFS.
 */
esp_err_t system_usb_host_unmount_storage(void);

/**
 * @brief Perform OTA firmware upgrade from USB flash drive.
 *
 * Mounts the U-disk, locates the OTA binary,
 * calls esp_msc_ota_begin / perform / end sequence.
 * Caller must call esp_restart() after this returns ESP_OK.
 *
 * @param ota_bin_path Full VFS path to OTA binary, e.g. "/usb/ate_fw.bin"
 */
esp_err_t system_usb_host_start_ota(const char *ota_bin_path);

/**
 * @brief Open CDC-ACM serial device (e.g. USB-to-UART adapter or DUT).
 *
 * Calls driver_usb_host_cdc_open().
 */
esp_err_t system_usb_host_open_serial(void);

/**
 * @brief Close CDC-ACM serial device.
 */
esp_err_t system_usb_host_close_serial(void);

#ifdef __cplusplus
}
#endif
