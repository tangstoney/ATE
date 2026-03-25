#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_i2c_module_t *driver_i2c_module_handle_t;

/*
 * ESP-IDF master-bus APIs used by this driver are already thread-safe.
 * Keep synchronization at higher semantic layers if state caching is needed.
 */
esp_err_t driver_i2c_module_bus_create(void);
esp_err_t driver_i2c_module_bus_destroy(void);

esp_err_t driver_i2c_module_create(driver_i2c_module_handle_t *out_handle);
esp_err_t driver_i2c_module_destroy(driver_i2c_module_handle_t handle);

esp_err_t driver_i2c_module_write(driver_i2c_module_handle_t handle,
                                  const uint8_t *data,
                                  size_t len);
esp_err_t driver_i2c_module_read(driver_i2c_module_handle_t handle,
                                 uint8_t *out_data,
                                 size_t len);
esp_err_t driver_i2c_module_write_read(driver_i2c_module_handle_t handle,
                                       const uint8_t *write_data,
                                       size_t write_len,
                                       uint8_t *read_data,
                                       size_t read_len);

esp_err_t driver_i2c_module_probe(driver_i2c_module_handle_t handle);

#ifdef __cplusplus
}
#endif
