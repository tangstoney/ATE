#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB Host Library (singleton).
 *
 * Calls usb_host_install() internally.
 * Must be called before any Host class driver.
 * Spawns the USB host library event task internally.
 */
esp_err_t driver_usb_host_init(void);

/**
 * @brief Deinitialize USB Host Library.
 *
 * All class drivers must be uninstalled before calling this.
 */
esp_err_t driver_usb_host_deinit(void);

/**
 * @brief Install MSC Host class driver and mount USB flash drive.
 *
 * Uses usb_host_msc driver (BOT + Transparent SCSI).
 * Registers the device on the VFS at mount_point.
 *
 * @param mount_point VFS path, e.g. "/usb"
 */
esp_err_t driver_usb_host_msc_mount(const char *mount_point);

/**
 * @brief Unmount MSC device and uninstall MSC Host driver.
 */
esp_err_t driver_usb_host_msc_unmount(void);

/**
 * @brief Install CDC-ACM Host class driver and open first available device.
 *
 * Uses usb_host_cdc_acm driver.
 */
esp_err_t driver_usb_host_cdc_open(void);

/**
 * @brief Close CDC-ACM device and uninstall CDC Host driver.
 */
esp_err_t driver_usb_host_cdc_close(void);

#ifdef __cplusplus
}
#endif
