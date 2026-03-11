#pragma once

#include "driver_display.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t system_display_get_panel_handles(esp_lcd_panel_handle_t *out_panel,
                                           esp_lcd_panel_io_handle_t *out_io);

#ifdef __cplusplus
}
#endif
