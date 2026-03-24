#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_display *driver_display_handle_t;

typedef struct {
    uint16_t hor_res;
    uint16_t ver_res;
    bool touch_available;
} driver_display_info_t; // 句柄，描述对象并间接控制用

esp_err_t driver_display_create(driver_display_handle_t *out_handle);
esp_err_t driver_display_start(driver_display_handle_t handle);
esp_err_t driver_display_destroy(driver_display_handle_t handle);
esp_err_t driver_display_get_info(driver_display_handle_t handle, driver_display_info_t *out_info);
esp_err_t driver_display_get_resolution(driver_display_handle_t handle, uint16_t *out_hor_res, uint16_t *out_ver_res);
esp_err_t driver_display_read_touch(driver_display_handle_t handle, bool *pressed, uint16_t *x, uint16_t *y);
esp_err_t driver_display_lock(driver_display_handle_t handle, int32_t timeout_ms);
esp_err_t driver_display_unlock(driver_display_handle_t handle);
esp_err_t driver_display_set_hw_pattern(driver_display_handle_t handle, mipi_dsi_pattern_type_t pattern);

// esp_err_t lcd_display_bsp_gpio_init(void);// todo codex ，现在直接用board宏定义

#ifdef __cplusplus
}
#endif
