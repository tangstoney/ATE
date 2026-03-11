/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_err.h"
#include "esp_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle for Ethernet driver
 */
typedef struct driver_eth_t *driver_eth_handle_t;

/**
 * @brief Create Ethernet driver (MAC/PHY init + install)
 */
esp_err_t driver_eth_create(driver_eth_handle_t *out_handle);

/**
 * @brief Destroy Ethernet driver
 */
esp_err_t driver_eth_destroy(driver_eth_handle_t handle);

/**
 * @brief Start Ethernet driver
 */
esp_err_t driver_eth_start(driver_eth_handle_t handle);

/**
 * @brief Stop Ethernet driver
 */
esp_err_t driver_eth_stop(driver_eth_handle_t handle);

/**
 * @brief Get raw esp_eth_handle_t for esp_netif glue
 */
esp_err_t driver_eth_get_raw_handle(driver_eth_handle_t handle, esp_eth_handle_t *out_eth);

#ifdef __cplusplus
}
#endif
