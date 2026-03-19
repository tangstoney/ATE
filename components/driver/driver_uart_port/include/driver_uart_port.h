#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_uart_port *driver_uart_port_handle_t;

typedef enum {
    DRIVER_UART_MODE_NORMAL = 0,
    DRIVER_UART_MODE_RS485_HALF,
} driver_uart_mode_t;

typedef struct {
    uart_port_t port;
    int tx_io;
    int rx_io;
    int rts_io;
    uint32_t baud_rate;
    driver_uart_mode_t mode;
    int rx_buf_size;
    int tx_buf_size;
    int timeout_ms;
} driver_uart_port_config_t;

esp_err_t driver_uart_port_create(const driver_uart_port_config_t *config,
                                  driver_uart_port_handle_t *out_handle);
esp_err_t driver_uart_port_delete(driver_uart_port_handle_t handle);

esp_err_t driver_uart_port_set_baudrate(driver_uart_port_handle_t handle,
                                        uint32_t baud_rate);
esp_err_t driver_uart_port_reconfigure(driver_uart_port_handle_t handle,
                                       const driver_uart_port_config_t *config);
esp_err_t driver_uart_port_recover(driver_uart_port_handle_t handle);

esp_err_t driver_uart_write(driver_uart_port_handle_t handle,
                            const uint8_t *data,
                            size_t len);
esp_err_t driver_uart_read(driver_uart_port_handle_t handle,
                           uint8_t *buf,
                           size_t len,
                           size_t *out_len);
esp_err_t driver_uart_flush(driver_uart_port_handle_t handle);

#ifdef __cplusplus
}
#endif
