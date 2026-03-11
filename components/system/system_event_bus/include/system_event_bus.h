#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_EVENT_NONE = 0,
    SYSTEM_EVENT_TOUCH,
    SYSTEM_EVENT_NETWORK_UP,
    SYSTEM_EVENT_NETWORK_DOWN,
} system_event_id_t;

typedef struct {
    system_event_id_t id;
    int32_t arg0;
    int32_t arg1;
} system_event_t;

esp_err_t system_event_bus_init(void);
esp_err_t system_event_bus_post(const system_event_t *event);
esp_err_t system_event_bus_get(system_event_t *event, int timeout_ms);

#ifdef __cplusplus
}
#endif
