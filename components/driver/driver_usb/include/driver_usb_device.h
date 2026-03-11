#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and install USB Device Stack (TinyUSB).
 *
 * Calls tinyusb_driver_install() with default config.
 * Must be called before enabling any USB Device class.
 */
esp_err_t driver_usb_device_init(void);

/**
 * @brief Stop USB Device Stack and uninstall TinyUSB driver.
 */
esp_err_t driver_usb_device_deinit(void);

/**
 * @brief Enable CDC-ACM USB Serial Device class.
 *
 * Calls tusb_cdc_acm_init() with default port TINYUSB_CDC_ACM_0.
 * Must be called after driver_usb_device_init().
 */
esp_err_t driver_usb_device_cdc_enable(void);

/**
 * @brief Enable MSC USB Device class backed by SPI Flash.
 *
 * Calls tinyusb_msc_storage_init_spiflash().
 * Must be called after driver_usb_device_init().
 */
esp_err_t driver_usb_device_msc_enable_spiflash(void);

/**
 * @brief Enable MSC USB Device class backed by SD/MMC card.
 *
 * Calls tinyusb_msc_storage_init_sdmmc().
 * Must be called after driver_usb_device_init().
 */
esp_err_t driver_usb_device_msc_enable_sdmmc(void);

#ifdef __cplusplus
}
#endif
