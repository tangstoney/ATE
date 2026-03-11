#include "driver_display_touch.h"

#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"

#define TOUCH_SCL_GPIO          GPIO_NUM_8
#define TOUCH_SDA_GPIO          GPIO_NUM_9
#define TOUCH_INT_GPIO          GPIO_NUM_10
#define TOUCH_RST_GPIO          GPIO_NUM_11
#define TOUCH_I2C_PORT          I2C_NUM_0

static const char *TAG = "drv_disp_touch";

esp_lcd_touch_handle_t driver_display_touch_init(uint16_t hor_res, uint16_t ver_res)
{
    esp_err_t ret = ESP_OK;
    static i2c_master_bus_handle_t i2c_bus = NULL;
    static esp_lcd_panel_io_handle_t touch_io = NULL;
    static esp_lcd_touch_handle_t touch_handle = NULL;
    bool created_i2c_bus = false;
    bool created_touch_io = false;
    bool created_touch = false;

    if (touch_handle != NULL) {
        return touch_handle;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TOUCH_I2C_PORT,
        .scl_io_num = TOUCH_SCL_GPIO,
        .sda_io_num = TOUCH_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_GOTO_ON_ERROR(i2c_new_master_bus(&bus_cfg, &i2c_bus), err, TAG, "i2c_new_master_bus failed");
    created_i2c_bus = true;

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.scl_speed_hz = 400 * 1000;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &io_cfg, &touch_io), err, TAG,
                      "esp_lcd_new_panel_io_i2c failed");
    created_touch_io = true;

    esp_lcd_touch_io_gt911_config_t gt911_extra = {
        .dev_addr = 0x5D,
    };

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = hor_res,
        .y_max = ver_res,
        .rst_gpio_num = TOUCH_RST_GPIO,
        .int_gpio_num = TOUCH_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
        .driver_data = &gt911_extra,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_touch_new_i2c_gt911(touch_io, &tp_cfg, &touch_handle), err, TAG,
                      "esp_lcd_touch_new_i2c_gt911 failed");
    created_touch = true;

    ESP_LOGI(TAG, "GT911 touch init complete");
    return touch_handle;

err:
    if (created_touch && touch_handle) {
        esp_lcd_touch_del(touch_handle);
        touch_handle = NULL;
    }
    if (created_touch_io && touch_io) {
        esp_lcd_panel_io_del(touch_io);
        touch_io = NULL;
    }
    if (created_i2c_bus && i2c_bus) {
        i2c_del_master_bus(i2c_bus);
        i2c_bus = NULL;
    }
    return NULL;
}
