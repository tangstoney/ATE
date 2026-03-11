#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_network *driver_network_handle_t;

esp_err_t driver_network_create(driver_network_handle_t *out_handle);
esp_err_t driver_network_destroy(driver_network_handle_t handle);
esp_err_t driver_network_send(driver_network_handle_t handle, const uint8_t *data, size_t len);
esp_err_t driver_network_recv(driver_network_handle_t handle, uint8_t *buf, size_t buf_len, size_t *out_len);

#ifdef __cplusplus
}
#endif
