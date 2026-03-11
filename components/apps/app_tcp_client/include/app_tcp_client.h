/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize TCP client application
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_FAIL: Initialization failed
 */
esp_err_t app_tcp_client_init(void);

/**
 * @brief Start TCP client task
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_FAIL: Start failed
 */
esp_err_t app_tcp_client_start(void);

/**
 * @brief Stop TCP client task
 */
esp_err_t app_tcp_client_stop(void);

#ifdef __cplusplus
}
#endif
