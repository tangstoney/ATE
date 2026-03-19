#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "system_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_MODULE_SERVICE_MAX_MODULES    16

typedef struct {
    uint32_t link_id;
    uint16_t scan_start_addr;
    uint16_t scan_end_addr;
    uint8_t default_protocol_device_id;
    uint32_t device_speed_hz;
    int io_timeout_ms;
} system_module_service_config_t;

typedef struct {
    uint16_t i2c_addr;
    uint8_t protocol_device_id;
    bool attached;
    bool online;
    uint8_t last_command;
    system_protocol_status_t last_status;
    uint32_t tx_count;
    uint32_t rx_count;
    esp_err_t last_err;
} system_module_info_t;

typedef struct {
    bool online;
    uint8_t last_command;
    system_protocol_status_t last_status;
    uint32_t tx_count;
    uint32_t rx_count;
    esp_err_t last_err;
} system_module_state_t;

typedef struct {
    uint16_t i2c_addr;
    uint8_t protocol_device_id;
    uint8_t command;
    size_t tx_len;
    size_t rx_len;
    esp_err_t result;
} system_module_xfer_event_t;

esp_err_t system_module_service_init(const system_module_service_config_t *config);
esp_err_t system_module_service_deinit(void);

esp_err_t system_module_scan(void);
esp_err_t system_module_get_count(size_t *out_count);
esp_err_t system_module_get_info(size_t index, system_module_info_t *out_info);
esp_err_t system_module_attach(uint16_t i2c_addr, uint8_t protocol_device_id);

esp_err_t system_module_send_cmd(uint16_t i2c_addr,
                                 uint8_t protocol_device_id,
                                 uint8_t command,
                                 const uint8_t *payload,
                                 uint16_t payload_len);
esp_err_t system_module_exec_action(uint16_t i2c_addr,
                                    uint8_t protocol_device_id,
                                    uint8_t command,
                                    const uint8_t *payload,
                                    uint16_t payload_len,
                                    uint8_t *response_buf,
                                    size_t response_buf_size,
                                    size_t expected_response_len,
                                    system_protocol_frame_t *out_frame);

esp_err_t system_module_get_state(uint16_t i2c_addr, system_module_state_t *out_state);

#ifdef __cplusplus
}
#endif
