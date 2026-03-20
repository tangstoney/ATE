#pragma once

#include <stdint.h>

#include "driver_i2c_master.h"
#include "esp_err.h"
#include "system_uart_link.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_COMM_MGR_MAX_I2C_LINKS    4
#define SYSTEM_COMM_MGR_MAX_UART_LINKS   4

typedef struct {
    uint32_t link_id;
    driver_i2c_master_config_t bus_config;
} system_comm_mgr_i2c_link_config_t;

esp_err_t system_comm_mgr_init(void);
esp_err_t system_comm_mgr_deinit(void);

esp_err_t system_comm_mgr_register_i2c_link(const system_comm_mgr_i2c_link_config_t *config,
                                            driver_i2c_master_handle_t *out_handle);
esp_err_t system_comm_mgr_unregister_i2c_link(driver_i2c_master_handle_t handle);
esp_err_t system_comm_mgr_get_i2c_link(uint32_t link_id, driver_i2c_master_handle_t *out_handle);

esp_err_t system_comm_mgr_register_uart_link(const system_uart_link_config_t *config,
                                             system_uart_link_handle_t *out_handle);
esp_err_t system_comm_mgr_unregister_uart_link(system_uart_link_handle_t handle);
esp_err_t system_comm_mgr_get_uart_link(uint32_t link_id, system_uart_link_handle_t *out_handle);

#ifdef __cplusplus
}
#endif
