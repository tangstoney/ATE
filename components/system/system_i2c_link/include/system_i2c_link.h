#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver_i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct system_i2c_link *system_i2c_link_handle_t;

typedef struct {
    uint32_t link_id;
    driver_i2c_master_config_t bus_config;
} system_i2c_link_config_t;

esp_err_t system_i2c_link_create(const system_i2c_link_config_t *config,
                                 system_i2c_link_handle_t *out_handle);
esp_err_t system_i2c_link_delete(system_i2c_link_handle_t handle);
esp_err_t system_i2c_link_get_id(system_i2c_link_handle_t handle, uint32_t *out_link_id);

esp_err_t system_i2c_link_probe(system_i2c_link_handle_t handle,
                                uint16_t dev_addr,
                                int timeout_ms);
esp_err_t system_i2c_link_recover(system_i2c_link_handle_t handle);

esp_err_t system_i2c_link_device_add(system_i2c_link_handle_t handle,
                                     const driver_i2c_device_config_t *config,
                                     driver_i2c_device_handle_t *out_dev);
esp_err_t system_i2c_link_device_remove(driver_i2c_device_handle_t dev);

esp_err_t system_i2c_link_write(driver_i2c_device_handle_t dev,
                                const uint8_t *data,
                                size_t len);
esp_err_t system_i2c_link_read(driver_i2c_device_handle_t dev,
                               uint8_t *buf,
                               size_t len);
esp_err_t system_i2c_link_write_read(driver_i2c_device_handle_t dev,
                                     const uint8_t *write_buf,
                                     size_t write_len,
                                     uint8_t *read_buf,
                                     size_t read_len);

#ifdef __cplusplus
}
#endif
