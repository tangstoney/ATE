#pragma once

#include <stdint.h>

#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

// todo 这个接口看起来有点问题，按理来说system和上级应该用不上esp_lcd_panel_handle_t  esp_lcd_panel_io_handle_t 
typedef struct {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    uint16_t hor_res;
    uint16_t ver_res;
} driver_display_lcd_handle_t;

driver_display_lcd_handle_t *driver_display_lcd_init(void);

#ifdef __cplusplus
}
#endif
