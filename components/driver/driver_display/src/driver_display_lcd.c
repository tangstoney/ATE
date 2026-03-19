#include "driver_display_lcd.h"

#include <stdbool.h>

#include "board_ate_p4.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"

#include "esp_lcd_jd9365_waveshare.h"

static const char *TAG = "drv_disp_lcd";

driver_display_lcd_handle_t *driver_display_lcd_init(void)
{
    esp_err_t ret = ESP_OK;
    (void)ret;   // 告诉编译器“我故意不再使用它”，不会再有未使用警告
    static bool initialized = false;
    static driver_display_lcd_handle_t handle;
    static esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
    bool created_ldo_chan = false;
    bool created_dsi_bus = false;
    bool created_panel_io = false;
    bool created_panel = false;

    if (initialized) {
        return &handle;
    }

    if (!phy_pwr_chan) {
        esp_ldo_channel_config_t ldo_cfg = {
            .chan_id = BOARD_LCD_MIPI_LDO_CHAN,
            .voltage_mv = BOARD_LCD_MIPI_LDO_MV,
        };
        ESP_GOTO_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan), err, TAG, "DSI PHY power failed");
        created_ldo_chan = true;
    }

    esp_lv_adapter_tear_avoid_mode_t tear_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL;
    esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_90;
    uint8_t num_fbs = esp_lv_adapter_get_required_frame_buffer_count(tear_mode, rotation);

    esp_lcd_dsi_bus_config_t bus_config = 
    {
        .bus_id = 0,
        .num_data_lanes = 2,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = 1500,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus), err, TAG, "esp_lcd_new_dsi_bus failed");
    created_dsi_bus = true;

    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &handle.io), err, TAG,
                      "esp_lcd_new_panel_io_dbi failed");
    created_panel_io = true;

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 80,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .pixel_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = BOARD_LCD_H_RES,
            .v_size = BOARD_LCD_V_RES,
            .hsync_back_porch = 20,
            .hsync_pulse_width = 20,
            .hsync_front_porch = 40,
            .vsync_back_porch = 10,
            .vsync_pulse_width = 4,
            .vsync_front_porch = 30,
        },
        .flags.use_dma2d = true,
        // .flags.disable_lp = true,
    };

    jd9365_vendor_config_t vendor_config = {
        .flags = {
            .use_mipi_interface = 1,
        },
        .mipi_config = {
            .dsi_bus = dsi_bus,
            .dpi_config = &dpi_config,
            .lane_num = 2,
        },
    };

    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 16,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .reset_gpio_num = BOARD_GPIO_LCD_RESET,
        .vendor_config = &vendor_config,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_jd9365(handle.io, &panel_config, &handle.panel), err, TAG,
                      "esp_lcd_new_panel_jd9365 failed");
    created_panel = true;
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(handle.panel), err, TAG, "esp_lcd_panel_reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(handle.panel), err, TAG, "esp_lcd_panel_init failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(handle.panel, true), err, TAG, "esp_lcd_panel_disp_on_off failed");

    handle.hor_res = BOARD_LCD_H_RES;
    handle.ver_res = BOARD_LCD_V_RES;
    initialized = true;
    return &handle;

err:
    if (created_panel && handle.panel) {
        esp_lcd_panel_del(handle.panel);
        handle.panel = NULL;
    }
    if (created_panel_io && handle.io) {
        esp_lcd_panel_io_del(handle.io);
        handle.io = NULL;
    }
    if (created_dsi_bus && dsi_bus) {
        esp_lcd_del_dsi_bus(dsi_bus);
        dsi_bus = NULL;
    }
    if (created_ldo_chan && phy_pwr_chan) {
        esp_ldo_release_channel(phy_pwr_chan);
        phy_pwr_chan = NULL;
    }
    return NULL;
}
