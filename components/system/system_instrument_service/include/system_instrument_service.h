#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_INSTRUMENT_STATE_UNINITIALIZED = 0,
    SYSTEM_INSTRUMENT_STATE_IDLE,
    SYSTEM_INSTRUMENT_STATE_NOT_SUPPORTED,
} system_instrument_state_t;

typedef struct {
    uint32_t link_id;
} system_instrument_service_config_t;

esp_err_t system_instrument_service_init(const system_instrument_service_config_t *config);
esp_err_t system_instrument_scan(void);
esp_err_t system_instrument_attach(uint32_t port_id);
esp_err_t system_instrument_send(uint8_t dev_id,
                                 uint8_t cmd,
                                 const uint8_t *payload,
                                 size_t len);
esp_err_t system_instrument_get_state(system_instrument_state_t *out_state);

#ifdef __cplusplus
}
#endif
