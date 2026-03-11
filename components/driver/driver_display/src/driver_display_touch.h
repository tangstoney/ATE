#pragma once

#include <stdint.h>

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_lcd_touch_handle_t driver_display_touch_init(uint16_t hor_res, uint16_t ver_res);

#ifdef __cplusplus
}
#endif
