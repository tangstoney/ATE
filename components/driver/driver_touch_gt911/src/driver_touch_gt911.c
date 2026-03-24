#include "driver_touch_gt911.h"

#include <stdlib.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "sdkconfig.h"

struct driver_touch_gt911_t {
    i2c_master_bus_handle_t i2c_bus;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_touch_handle_t touch_handle;
};

static const char *TAG = "driver_touch_gt911";
// Preserve legacy behavior: repeated create calls share one hardware instance.
static driver_touch_gt911_handle_t s_shared_handle;
static size_t s_ref_count;

#ifdef CONFIG_DRIVER_TOUCH_GT911_USE_INTERNAL_PULLUP
#define DRIVER_TOUCH_GT911_INTERNAL_PULLUP 1
#else
#define DRIVER_TOUCH_GT911_INTERNAL_PULLUP 0
#endif

// Touch orientation is calibrated independently from LCD rotation.
// x_max/y_max describe the raw touch coordinate range, while swap/mirror flags
// are used to align the GT911 sensor mounting with the displayed UI.
// Current board tuning keeps raw X/Y order and only applies optional mirroring.
#ifdef CONFIG_DRIVER_TOUCH_GT911_SWAP_XY
#define DRIVER_TOUCH_GT911_SWAP_XY_ENABLED 1
#else
#define DRIVER_TOUCH_GT911_SWAP_XY_ENABLED 0
#endif

#ifdef CONFIG_DRIVER_TOUCH_GT911_MIRROR_X
#define DRIVER_TOUCH_GT911_MIRROR_X_ENABLED 0
#else
#define DRIVER_TOUCH_GT911_MIRROR_X_ENABLED 0
#endif

#ifdef CONFIG_DRIVER_TOUCH_GT911_MIRROR_Y
#define DRIVER_TOUCH_GT911_MIRROR_Y_ENABLED 1
#else
#define DRIVER_TOUCH_GT911_MIRROR_Y_ENABLED 0
#endif

esp_err_t driver_touch_gt911_create(driver_touch_gt911_handle_t *out_handle)
{
    driver_touch_gt911_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    if (s_shared_handle) {
        s_ref_count++;
        *out_handle = s_shared_handle;
        return ESP_OK;
    }

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = CONFIG_DRIVER_TOUCH_GT911_I2C_PORT,
        .scl_io_num = CONFIG_DRIVER_TOUCH_GT911_I2C_SCL_GPIO,
        .sda_io_num = CONFIG_DRIVER_TOUCH_GT911_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = DRIVER_TOUCH_GT911_INTERNAL_PULLUP,
    };
    ESP_GOTO_ON_ERROR(i2c_new_master_bus(&bus_cfg, &handle->i2c_bus), err, TAG,
                      "i2c_new_master_bus failed");

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.dev_addr = CONFIG_DRIVER_TOUCH_GT911_I2C_ADDR;
    io_cfg.scl_speed_hz = CONFIG_DRIVER_TOUCH_GT911_I2C_CLK_HZ;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_i2c(handle->i2c_bus, &io_cfg, &handle->io_handle), err, TAG,
                      "esp_lcd_new_panel_io_i2c failed");

    esp_lcd_touch_io_gt911_config_t gt911_extra = {
        .dev_addr = CONFIG_DRIVER_TOUCH_GT911_I2C_ADDR,
    };

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_DRIVER_TOUCH_GT911_X_MAX,
        .y_max = CONFIG_DRIVER_TOUCH_GT911_Y_MAX,
        .rst_gpio_num = CONFIG_DRIVER_TOUCH_GT911_RESET_GPIO,
        .int_gpio_num = CONFIG_DRIVER_TOUCH_GT911_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = DRIVER_TOUCH_GT911_SWAP_XY_ENABLED,
            .mirror_x = DRIVER_TOUCH_GT911_MIRROR_X_ENABLED,
            .mirror_y = DRIVER_TOUCH_GT911_MIRROR_Y_ENABLED,
        },
        .driver_data = &gt911_extra,
    };

    ESP_LOGI(TAG,
             "touch map: x_max=%d y_max=%d swap_xy=%d mirror_x=%d mirror_y=%d",
             CONFIG_DRIVER_TOUCH_GT911_X_MAX,
             CONFIG_DRIVER_TOUCH_GT911_Y_MAX,
             DRIVER_TOUCH_GT911_SWAP_XY_ENABLED,
             DRIVER_TOUCH_GT911_MIRROR_X_ENABLED,
             DRIVER_TOUCH_GT911_MIRROR_Y_ENABLED);

    ESP_GOTO_ON_ERROR(esp_lcd_touch_new_i2c_gt911(handle->io_handle, &tp_cfg, &handle->touch_handle), err, TAG,
                      "esp_lcd_touch_new_i2c_gt911 failed");

    s_shared_handle = handle;
    s_ref_count = 1;
    *out_handle = handle;
    return ESP_OK;

err:
    if (handle) {
        if (handle->touch_handle) {
            esp_lcd_touch_del(handle->touch_handle);
        }
        if (handle->io_handle) {
            esp_lcd_panel_io_del(handle->io_handle);
        }
        if (handle->i2c_bus) {
            i2c_del_master_bus(handle->i2c_bus);
        }
        free(handle);
    }
    return ret;
}

esp_err_t driver_touch_gt911_destroy(driver_touch_gt911_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle && handle == s_shared_handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");

    if (s_ref_count > 0) {
        s_ref_count--;
    }
    return ESP_OK;
}

esp_err_t driver_touch_gt911_read_data(driver_touch_gt911_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle && handle->touch_handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return esp_lcd_touch_read_data(handle->touch_handle);
}

esp_err_t driver_touch_gt911_get_data(driver_touch_gt911_handle_t handle,
                                      bool *pressed,
                                      uint16_t *x,
                                      uint16_t *y)
{
    esp_lcd_touch_point_data_t points[1] = {0};
    uint8_t count = 0;

    ESP_RETURN_ON_FALSE(handle && handle->touch_handle && pressed && x && y,
                        ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(esp_lcd_touch_get_data(handle->touch_handle, points, &count, 1), TAG,
                        "esp_lcd_touch_get_data failed");

    *pressed = count > 0;
    *x = count > 0 ? points[0].x : 0;
    *y = count > 0 ? points[0].y : 0;
    return ESP_OK;
}

esp_err_t driver_touch_gt911_get_native_handle(driver_touch_gt911_handle_t handle,
                                               void **out_native_handle)
{
    ESP_RETURN_ON_FALSE(handle && out_native_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_native_handle = handle->touch_handle;
    return ESP_OK;
}
