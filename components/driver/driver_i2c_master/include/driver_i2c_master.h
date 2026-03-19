#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_i2c_master *driver_i2c_master_handle_t;
typedef struct driver_i2c_device *driver_i2c_device_handle_t;

typedef struct {
    i2c_port_num_t port;
    int sda_io;
    int scl_io;
    uint32_t clk_speed_hz;
    int timeout_ms;
    bool enable_internal_pullup;
    uint8_t glitch_ignore_cnt;
    i2c_clock_source_t clk_source;
} driver_i2c_master_config_t;

typedef struct {
    uint16_t dev_addr;
    uint32_t scl_speed_hz;
    int timeout_ms;
    i2c_addr_bit_len_t addr_bit_len;
} driver_i2c_device_config_t;

esp_err_t driver_i2c_master_create(const driver_i2c_master_config_t *config,
                                   driver_i2c_master_handle_t *out_handle);
esp_err_t driver_i2c_master_delete(driver_i2c_master_handle_t handle);

esp_err_t driver_i2c_master_probe(driver_i2c_master_handle_t handle,
                                  uint16_t dev_addr,
                                  int timeout_ms);
esp_err_t driver_i2c_master_recover(driver_i2c_master_handle_t handle);

esp_err_t driver_i2c_device_add(driver_i2c_master_handle_t bus,
                                const driver_i2c_device_config_t *config,
                                driver_i2c_device_handle_t *out_dev);
esp_err_t driver_i2c_device_remove(driver_i2c_device_handle_t dev);

esp_err_t driver_i2c_write(driver_i2c_device_handle_t dev,
                           const uint8_t *data,
                           size_t len);
esp_err_t driver_i2c_read(driver_i2c_device_handle_t dev,
                          uint8_t *buf,
                          size_t len);
esp_err_t driver_i2c_write_read(driver_i2c_device_handle_t dev,
                                const uint8_t *write_buf,
                                size_t write_len,
                                uint8_t *read_buf,
                                size_t read_len);

#ifdef __cplusplus
}
#endif
