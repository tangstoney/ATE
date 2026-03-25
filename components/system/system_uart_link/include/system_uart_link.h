#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver_uart_instrument.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct system_uart_link *system_uart_link_handle_t;

typedef struct {
    uint32_t link_id;
} system_uart_link_config_t;

esp_err_t system_uart_link_create(const system_uart_link_config_t *config,
                                  system_uart_link_handle_t *out_handle);
esp_err_t system_uart_link_delete(system_uart_link_handle_t handle);
esp_err_t system_uart_link_get_id(system_uart_link_handle_t handle, uint32_t *out_link_id);

esp_err_t system_uart_link_write(system_uart_link_handle_t handle,
                                 const uint8_t *data,
                                 size_t len);
esp_err_t system_uart_link_read(system_uart_link_handle_t handle,
                                uint8_t *buf,
                                size_t len,
                                size_t *out_len);

#ifdef __cplusplus
}
#endif
