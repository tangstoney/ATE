#include "driver_display.h"

#include <stdlib.h>

#include "esp_check.h"
#include "esp_lcd_touch.h"

#include "driver_display_lcd.h"
#include "driver_display_touch.h"

typedef struct driver_display {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_touch_handle_t touch_handle;
    bool touch_available;
    driver_display_info_t info;
} driver_display_t;

static const char *TAG = "driver_display";

esp_err_t driver_display_create(driver_display_handle_t *out_handle)
{
    esp_err_t ret = ESP_OK;           // ← 必须加这一行
    driver_display_t *handle = NULL;
    ESP_GOTO_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, err, TAG, "out_handle is NULL");
    
    handle = calloc(1, sizeof(driver_display_t));
    ESP_GOTO_ON_FALSE(handle, ESP_ERR_NO_MEM, err, TAG, "no memory");

    driver_display_lcd_handle_t *disp = driver_display_lcd_init();
    ESP_GOTO_ON_FALSE(disp, ESP_FAIL, err, TAG, "driver_display_lcd_init failed");

    handle->panel = disp->panel;
    handle->io = disp->io;
    handle->touch_handle = driver_display_touch_init(disp->hor_res, disp->ver_res);
    handle->touch_available = (handle->touch_handle != NULL);
    handle->info.width = disp->hor_res;
    handle->info.height = disp->ver_res;
    handle->info.touch_available = handle->touch_available;

    *out_handle = handle;
    return ESP_OK;

err:
    free(handle);
    return ESP_FAIL;
}

esp_err_t driver_display_destroy(driver_display_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

esp_err_t driver_display_get_info(driver_display_handle_t handle, driver_display_info_t *out_info)
{
    ESP_RETURN_ON_FALSE(handle && out_info, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_info = handle->info;
    return ESP_OK;
}

esp_err_t driver_display_get_panel_handle(driver_display_handle_t handle,
                                          esp_lcd_panel_handle_t *out_panel,
                                          esp_lcd_panel_io_handle_t *out_io)
{
    ESP_RETURN_ON_FALSE(handle && out_panel && out_io, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_panel = handle->panel;
    *out_io = handle->io;
    return ESP_OK;
}

esp_err_t driver_display_get_resolution(driver_display_handle_t handle, uint16_t *w, uint16_t *h)
{
    ESP_RETURN_ON_FALSE(handle && w && h, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *w = handle->info.width;
    *h = handle->info.height;
    return ESP_OK;
}

esp_err_t driver_display_get_touch_handle(driver_display_handle_t handle, esp_lcd_touch_handle_t *out_touch)
{
    ESP_RETURN_ON_FALSE(handle && out_touch, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_touch = handle->touch_handle;
    return ESP_OK;
}

esp_err_t driver_display_read_touch(driver_display_handle_t handle, bool *pressed, uint16_t *x, uint16_t *y)
{
    ESP_RETURN_ON_FALSE(handle && pressed && x && y, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(handle->touch_handle, ESP_ERR_INVALID_STATE, TAG, "touch unavailable");

    ESP_RETURN_ON_ERROR(esp_lcd_touch_read_data(handle->touch_handle), TAG, "read touch failed");

    esp_lcd_touch_point_data_t points[1] = {0};
    uint8_t count = 0;
    ESP_RETURN_ON_ERROR(esp_lcd_touch_get_data(handle->touch_handle, points, &count, 1), TAG,
                        "get touch data failed");

    *pressed = (count > 0);
    *x = (count > 0) ? points[0].x : 0;
    *y = (count > 0) ? points[0].y : 0;
    return ESP_OK;
}
