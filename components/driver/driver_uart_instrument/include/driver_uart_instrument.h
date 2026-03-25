#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_UART_INSTRUMENT_MAX 4U

typedef struct driver_uart_instrument_t *driver_uart_instrument_handle_t;

/*
 * This driver owns fixed board-level UART slots. Passing timeout_ms=0 uses the
 * slot default timeout internally.
 */
esp_err_t driver_uart_instrument_install_all(void);
esp_err_t driver_uart_instrument_install(uint8_t slot_index,
                                         driver_uart_instrument_handle_t *out_handle);
esp_err_t driver_uart_instrument_uninstall(driver_uart_instrument_handle_t handle);

esp_err_t driver_uart_instrument_write(driver_uart_instrument_handle_t handle,
                                       const uint8_t *data,
                                       size_t len);
esp_err_t driver_uart_instrument_read(driver_uart_instrument_handle_t handle,
                                      uint8_t *out_data,
                                      size_t len,
                                      uint32_t timeout_ms);
esp_err_t driver_uart_instrument_wait_tx_done(driver_uart_instrument_handle_t handle,
                                              uint32_t timeout_ms);
esp_err_t driver_uart_instrument_flush_rx(driver_uart_instrument_handle_t handle);
esp_err_t driver_uart_instrument_set_loopback(driver_uart_instrument_handle_t handle,
                                              bool enable);

#ifdef __cplusplus
}
#endif
