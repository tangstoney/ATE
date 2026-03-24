#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_lcd_panel_t *driver_lcd_panel_handle_t;

esp_err_t driver_lcd_panel_create(driver_lcd_panel_handle_t *out_handle);
esp_err_t driver_lcd_panel_create_with_frame_buffers(driver_lcd_panel_handle_t *out_handle, uint8_t num_fbs);
esp_err_t driver_lcd_panel_destroy(driver_lcd_panel_handle_t handle);
esp_err_t driver_lcd_panel_flush(driver_lcd_panel_handle_t handle,
                                 int x1,
                                 int y1,
                                 int x2,
                                 int y2,
                                 const void *color_data);
esp_err_t driver_lcd_panel_disp_on_off(driver_lcd_panel_handle_t handle, bool on);
esp_err_t driver_lcd_panel_get_resolution(driver_lcd_panel_handle_t handle,
                                          uint16_t *out_hor_res,
                                          uint16_t *out_ver_res);
esp_err_t driver_lcd_panel_get_handles(driver_lcd_panel_handle_t handle,
                                       esp_lcd_panel_handle_t *out_panel,
                                       esp_lcd_panel_io_handle_t *out_io);

#ifdef __cplusplus
}
#endif
