#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Placeholder for the future module-bus selector / mux layer.
 *
 * Current stage:
 * - keeps only software-side selection state
 * - does not drive any hardware selector yet
 * - is not wired into system_module yet
 */

esp_err_t system_module_bus_init(void);
esp_err_t system_module_bus_deinit(void);

esp_err_t system_module_bus_select_channel(uint32_t channel_id);
esp_err_t system_module_bus_get_selected_channel(uint32_t *out_channel_id);
esp_err_t system_module_bus_is_initialized(bool *out_initialized);

#ifdef __cplusplus
}
#endif
