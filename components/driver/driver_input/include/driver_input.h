#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool pressed;
    uint16_t x;
    uint16_t y;
} driver_input_touch_point_t;

esp_err_t driver_input_init(void);
esp_err_t driver_input_read_touch(driver_input_touch_point_t *out_point);

#ifdef __cplusplus
}
#endif
