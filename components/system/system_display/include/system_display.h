#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t system_display_init(void);
typedef void (*system_display_ui_init_fn_t)(void);
esp_err_t system_display_register_ui_init_cb(system_display_ui_init_fn_t cb);
esp_err_t system_ui_init(void);
esp_err_t system_display_lock(void);
esp_err_t system_display_unlock(void);
esp_err_t system_display_get_resolution(uint16_t *out_hor_res, uint16_t *out_ver_res);
esp_err_t system_display_set_hw_pattern(mipi_dsi_pattern_type_t pattern);

#ifdef __cplusplus
}
#endif
