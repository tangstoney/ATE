#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_touch_gt911_t *driver_touch_gt911_handle_t;

esp_err_t driver_touch_gt911_create(driver_touch_gt911_handle_t *out_handle);
esp_err_t driver_touch_gt911_destroy(driver_touch_gt911_handle_t handle);
esp_err_t driver_touch_gt911_read_data(driver_touch_gt911_handle_t handle);
esp_err_t driver_touch_gt911_get_data(driver_touch_gt911_handle_t handle,
                                      bool *pressed,
                                      uint16_t *x,
                                      uint16_t *y);
esp_err_t driver_touch_gt911_get_native_handle(driver_touch_gt911_handle_t handle,
                                               void **out_native_handle);

#ifdef __cplusplus
}
#endif
