#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
