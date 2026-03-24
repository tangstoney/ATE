#include "driver_lcd_panel.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_lcd_jd9365_waveshare.h"

struct driver_lcd_panel_t {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_dsi_bus_handle_t dsi_bus;
    esp_ldo_channel_handle_t phy_pwr_chan;
    uint16_t hor_res;
    uint16_t ver_res;
    uint8_t num_fbs;
};

static const char *TAG = "driver_lcd_panel";
// Preserve legacy behavior: repeated create calls share one hardware instance.
static driver_lcd_panel_handle_t s_shared_handle;
static size_t s_ref_count;
static uint8_t s_shared_num_fbs;

#ifdef CONFIG_DRIVER_LCD_USE_DMA2D
#define DRIVER_LCD_USE_DMA2D 1
#else
#define DRIVER_LCD_USE_DMA2D 0
#endif

#ifdef CONFIG_DRIVER_LCD_SWAP_XY
#define DRIVER_LCD_SWAP_XY_ENABLED 1
#else
#define DRIVER_LCD_SWAP_XY_ENABLED 0
#endif

#ifdef CONFIG_DRIVER_LCD_MIRROR_X
#define DRIVER_LCD_MIRROR_X_ENABLED 1
#else
#define DRIVER_LCD_MIRROR_X_ENABLED 0
#endif

#ifdef CONFIG_DRIVER_LCD_MIRROR_Y
#define DRIVER_LCD_MIRROR_Y_ENABLED 1
#else
#define DRIVER_LCD_MIRROR_Y_ENABLED 0
#endif

static esp_err_t driver_lcd_panel_set_output_high(gpio_num_t gpio_num)
{
    if (gpio_num < 0) {
        return ESP_OK;
    }

    gpio_config_t config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << gpio_num,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "gpio_config failed for io=%d", gpio_num);
    return gpio_set_level(gpio_num, 1);
}

static esp_err_t driver_lcd_panel_power_up(void)
{
    ESP_RETURN_ON_ERROR(driver_lcd_panel_set_output_high((gpio_num_t)CONFIG_DRIVER_LCD_POWER_EN_GPIO),
                        TAG, "power_en init failed");
    ESP_RETURN_ON_ERROR(driver_lcd_panel_set_output_high((gpio_num_t)CONFIG_DRIVER_LCD_ENABLE_GPIO),
                        TAG, "enable init failed");
    ESP_RETURN_ON_ERROR(driver_lcd_panel_set_output_high((gpio_num_t)CONFIG_DRIVER_LCD_BACKLIGHT_GPIO),
                        TAG, "backlight init failed");
    return driver_lcd_panel_set_output_high((gpio_num_t)CONFIG_DRIVER_LCD_RESET_GPIO);
}

esp_err_t driver_lcd_panel_create_with_frame_buffers(driver_lcd_panel_handle_t *out_handle, uint8_t num_fbs)
{
    driver_lcd_panel_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    ESP_RETURN_ON_FALSE(num_fbs >= 1 && num_fbs <= 3, ESP_ERR_INVALID_ARG, TAG, "invalid num_fbs=%u", num_fbs);

    if (s_shared_handle) {
        if (s_shared_num_fbs != num_fbs) {
            ESP_LOGW(TAG,
                     "panel already created with num_fbs=%u, ignore request=%u",
                     (unsigned)s_shared_num_fbs,
                     (unsigned)num_fbs);
        }
        s_ref_count++;
        *out_handle = s_shared_handle;
        return ESP_OK;
    }

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    ESP_GOTO_ON_ERROR(driver_lcd_panel_power_up(), err, TAG, "lcd gpio init failed");

    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = CONFIG_DRIVER_LCD_MIPI_LDO_CHAN,
        .voltage_mv = CONFIG_DRIVER_LCD_MIPI_LDO_MV,
    };
    ESP_GOTO_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &handle->phy_pwr_chan), err, TAG,
                      "DSI PHY power failed");

    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = CONFIG_DRIVER_LCD_DSI_BUS_ID,
        .num_data_lanes = CONFIG_DRIVER_LCD_DSI_LANE_NUM,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = CONFIG_DRIVER_LCD_DSI_LANE_BIT_RATE_MBPS,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &handle->dsi_bus), err, TAG,
                      "esp_lcd_new_dsi_bus failed");

    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = CONFIG_DRIVER_LCD_DBI_VIRTUAL_CHANNEL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(handle->dsi_bus, &dbi_config, &handle->io), err, TAG,
                      "esp_lcd_new_panel_io_dbi failed");

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = CONFIG_DRIVER_LCD_DPI_VIRTUAL_CHANNEL,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = CONFIG_DRIVER_LCD_DPI_CLK_MHZ,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .pixel_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = CONFIG_DRIVER_LCD_H_RES,
            .v_size = CONFIG_DRIVER_LCD_V_RES,
            .hsync_back_porch = CONFIG_DRIVER_LCD_HSYNC_BACK_PORCH,
            .hsync_pulse_width = CONFIG_DRIVER_LCD_HSYNC_PULSE_WIDTH,
            .hsync_front_porch = CONFIG_DRIVER_LCD_HSYNC_FRONT_PORCH,
            .vsync_back_porch = CONFIG_DRIVER_LCD_VSYNC_BACK_PORCH,
            .vsync_pulse_width = CONFIG_DRIVER_LCD_VSYNC_PULSE_WIDTH,
            .vsync_front_porch = CONFIG_DRIVER_LCD_VSYNC_FRONT_PORCH,
        },
        .flags.use_dma2d = DRIVER_LCD_USE_DMA2D,
    };

    jd9365_vendor_config_t vendor_config = {
        .flags = {
            .use_mipi_interface = 1,
        },
        .mipi_config = {
            .dsi_bus = handle->dsi_bus,
            .dpi_config = &dpi_config,
            .lane_num = CONFIG_DRIVER_LCD_DSI_LANE_NUM,
        },
    };

    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 16,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .reset_gpio_num = CONFIG_DRIVER_LCD_RESET_GPIO,
        .vendor_config = &vendor_config,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_jd9365(handle->io, &panel_config, &handle->panel), err, TAG,
                      "esp_lcd_new_panel_jd9365 failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(handle->panel), err, TAG, "esp_lcd_panel_reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(handle->panel), err, TAG, "esp_lcd_panel_init failed");
    esp_lcd_panel_swap_xy(handle->panel, DRIVER_LCD_SWAP_XY_ENABLED);
    esp_lcd_panel_mirror(handle->panel, DRIVER_LCD_MIRROR_X_ENABLED, DRIVER_LCD_MIRROR_Y_ENABLED);
    esp_lcd_panel_disp_on_off(handle->panel, true);

    handle->hor_res = CONFIG_DRIVER_LCD_H_RES;
    handle->ver_res = CONFIG_DRIVER_LCD_V_RES;
    handle->num_fbs = num_fbs;

    s_shared_handle = handle;
    s_ref_count = 1;
    s_shared_num_fbs = num_fbs;
    *out_handle = handle;
    return ESP_OK;

err:
    if (handle) {
        if (handle->panel) {
            esp_lcd_panel_del(handle->panel);
        }
        if (handle->io) {
            esp_lcd_panel_io_del(handle->io);
        }
        if (handle->dsi_bus) {
            esp_lcd_del_dsi_bus(handle->dsi_bus);
        }
        if (handle->phy_pwr_chan) {
            esp_ldo_release_channel(handle->phy_pwr_chan);
        }
        free(handle);
    }
    return ret;
}

esp_err_t driver_lcd_panel_create(driver_lcd_panel_handle_t *out_handle)
{
    return driver_lcd_panel_create_with_frame_buffers(out_handle, CONFIG_DRIVER_LCD_NUM_FBS);
}

esp_err_t driver_lcd_panel_destroy(driver_lcd_panel_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle && handle == s_shared_handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");

    if (s_ref_count > 0) {
        s_ref_count--;
    }
    return ESP_OK;
}

esp_err_t driver_lcd_panel_flush(driver_lcd_panel_handle_t handle,
                                 int x1,
                                 int y1,
                                 int x2,
                                 int y2,
                                 const void *color_data)
{
    ESP_RETURN_ON_FALSE(handle && handle->panel && color_data, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(x2 >= x1 && y2 >= y1, ESP_ERR_INVALID_ARG, TAG, "invalid area");
    return esp_lcd_panel_draw_bitmap(handle->panel, x1, y1, x2 + 1, y2 + 1, color_data);
}

esp_err_t driver_lcd_panel_disp_on_off(driver_lcd_panel_handle_t handle, bool on)
{
    ESP_RETURN_ON_FALSE(handle && handle->panel, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return esp_lcd_panel_disp_on_off(handle->panel, on);
}

esp_err_t driver_lcd_panel_get_resolution(driver_lcd_panel_handle_t handle,
                                          uint16_t *out_hor_res,
                                          uint16_t *out_ver_res)
{
    ESP_RETURN_ON_FALSE(handle && out_hor_res && out_ver_res, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_hor_res = handle->hor_res;
    *out_ver_res = handle->ver_res;
    return ESP_OK;
}

esp_err_t driver_lcd_panel_get_handles(driver_lcd_panel_handle_t handle,
                                       esp_lcd_panel_handle_t *out_panel,
                                       esp_lcd_panel_io_handle_t *out_io)
{
    ESP_RETURN_ON_FALSE(handle && out_panel && out_io, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_panel = handle->panel;
    *out_io = handle->io;
    return ESP_OK;
}
