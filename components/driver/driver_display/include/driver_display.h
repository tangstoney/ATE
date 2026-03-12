#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_display *driver_display_handle_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    bool touch_available;
} driver_display_info_t; // 句柄，描述对象并间接控制用

esp_err_t driver_display_create(driver_display_handle_t *out_handle);
esp_err_t driver_display_destroy(driver_display_handle_t handle);
esp_err_t driver_display_get_info(driver_display_handle_t handle, driver_display_info_t *out_info);
esp_err_t driver_display_get_panel_handle(driver_display_handle_t handle,
                                          esp_lcd_panel_handle_t *out_panel,
                                          esp_lcd_panel_io_handle_t *out_io);
esp_err_t driver_display_get_resolution(driver_display_handle_t handle, uint16_t *w, uint16_t *h);
esp_err_t driver_display_get_touch_handle(driver_display_handle_t handle, esp_lcd_touch_handle_t *out_touch);
esp_err_t driver_display_read_touch(driver_display_handle_t handle, bool *pressed, uint16_t *x, uint16_t *y);

esp_err_t lcd_display_bsp_gpio_init(void);// todo codex 

#ifdef __cplusplus
}
#endif
