#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start USB Device stack (TinyUSB).
 *
 * Calls driver_usb_device_init().
 * Must be called before enabling any USB Device class.
 */
esp_err_t system_usb_device_start(void);

/**
 * @brief Stop USB Device stack.
 *
 * Calls driver_usb_device_deinit().
 */
esp_err_t system_usb_device_stop(void);

/**
 * @brief Start CDC-ACM USB Serial Device.
 *
 * Enables the device to appear as a USB serial port on the host PC.
 * Calls driver_usb_device_cdc_enable().
 */
esp_err_t system_usb_device_start_serial(void);

/**
 * @brief Start MSC USB Storage Device (SPI Flash).
 *
 * Enables the device to appear as a USB flash drive on the host PC.
 * Calls driver_usb_device_msc_enable_spiflash().
 */
esp_err_t system_usb_device_start_storage(void);

#ifdef __cplusplus
}
#endif
