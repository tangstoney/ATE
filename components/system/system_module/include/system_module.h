#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Public Event Contract
 *
 * These declarations are the subscribable event API exposed by system_module.
 * App-layer aggregators such as app_module_runtime should include this header
 * and use these event ids / payload types with esp_event subscriptions.
 * -------------------------------------------------------------------------- */

ESP_EVENT_DECLARE_BASE(SYSTEM_MODULE_EVENT);

typedef enum {
    SYSTEM_MODULE_EVENT_ONLINE = 0,
    SYSTEM_MODULE_EVENT_OFFLINE,
    SYSTEM_MODULE_EVENT_STATUS_UPDATED,
    SYSTEM_MODULE_EVENT_COMMAND_DONE,
} system_module_event_id_t;

typedef struct {
    uint8_t state;
    uint16_t error_code;
} system_module_status_event_t;

typedef struct {
    uint8_t cmd;
    bool success;
} system_module_cmd_done_event_t;

/* --------------------------------------------------------------------------
 * Functional Module API
 *
 * The declarations below are the callable system_module capability surface.
 * Use these functions when you need to create, query or drive the module
 * service itself. Do not confuse them with the subscribable event ids above.
 * -------------------------------------------------------------------------- */

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
    uint8_t last_command_id;
    uint16_t detail_status_code;
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
