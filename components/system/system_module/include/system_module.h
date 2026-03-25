#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "system_module_events.h"
#include "system_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_MODULE_DEFAULT_DEVICE_ID    0x02U

typedef struct system_module_t *system_module_handle_t;

typedef enum {
    SYSTEM_MODULE_STATE_UNKNOWN = 0,
    SYSTEM_MODULE_STATE_OFFLINE,
    SYSTEM_MODULE_STATE_ONLINE,
    SYSTEM_MODULE_STATE_ERROR,
} system_module_state_t;

typedef struct {
    system_module_state_t state;
    uint16_t error_code;
    uint16_t i2c_addr;
    uint8_t protocol_device_id;
    uint8_t last_command;
    system_protocol_status_t last_protocol_status;
    uint32_t tx_count;
    uint32_t rx_count;
    esp_err_t last_err;
    bool attached;
    bool online;
} module_status_t;

esp_err_t system_module_create(system_module_handle_t *out_handle);
esp_err_t system_module_destroy(system_module_handle_t handle);

esp_err_t system_module_get_status(system_module_handle_t handle,
                                   module_status_t *out_status);
esp_err_t system_module_send_command(system_module_handle_t handle,
                                     uint8_t cmd,
                                     const uint8_t *payload,
                                     size_t len);
esp_err_t system_module_is_online(system_module_handle_t handle,
                                  bool *out_online);

#ifdef __cplusplus
}
#endif
