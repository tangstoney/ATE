#include "driver_input.h"

#include "esp_check.h"

#include "driver_display.h"

static const char *TAG = "driver_input";
static driver_display_handle_t s_display = NULL;

esp_err_t driver_input_init(void)
{
    if (s_display) {
        return ESP_OK;
    }
    return driver_display_create(&s_display);
}

esp_err_t driver_input_read_touch(driver_input_touch_point_t *out_point)
{
    ESP_RETURN_ON_FALSE(out_point, ESP_ERR_INVALID_ARG, TAG, "out_point is NULL");
    ESP_RETURN_ON_ERROR(driver_input_init(), TAG, "driver_input_init failed");

    bool pressed = false;
    uint16_t x = 0;
    uint16_t y = 0;
    ESP_RETURN_ON_ERROR(driver_display_read_touch(s_display, &pressed, &x, &y), TAG,
                        "driver_display_read_touch failed");

    out_point->pressed = pressed;
    out_point->x = x;
    out_point->y = y;
    return ESP_OK;
}
