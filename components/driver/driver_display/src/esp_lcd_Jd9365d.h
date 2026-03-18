#ifndef _ESP_LCD_JD9365D_H
#define _ESP_LCD_JD9365D_H

#include "soc/soc_caps.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JD9365D 屏驱动。
 * @note 深圳屏厂，10.1 Elite 同款。
 */

/**
 * @brief LCD panel initialization commands.
 *
 */
typedef struct {
    int cmd;                /*<! The specific LCD command */
    const void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} esp_lcd_jd9365_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const esp_lcd_jd9365_lcd_init_cmd_t *init_cmds;         /*!< Pointer to initialization commands array. Set to NULL if using default commands.
                                                     *   The array should be declared as `static const` and positioned outside the function.
                                                     *   Please refer to `vendor_specific_init_default` in source file.
                                                     */
    uint16_t init_cmds_size;                        /*<! Number of commands in above array */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;               /*!< MIPI-DSI bus configuration */
        const esp_lcd_dpi_panel_config_t *dpi_config;   /*!< MIPI-DPI panel configuration */
        uint8_t  lane_num;                              /*!< Number of MIPI-DSI lanes */
    } mipi_config;
    struct {
        unsigned int use_mipi_interface: 1;         /*<! Set to 1 if using MIPI interface, default is RGB interface */
        unsigned int mirror_by_cmd: 1;              /*<! The `mirror()` function will be implemented by LCD command if set to 1. This flag is only valid for the RGB interface.
                                                     *   Otherwise, the function will be implemented by software.
                                                     */

        union {
            unsigned int auto_del_panel_io: 1;
            unsigned int enable_io_multiplex: 1;
        };  /*<! Delete the panel IO instance automatically if set to 1. All `*_by_cmd` flags will be invalid.
             *   If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
             *   Please set it to 1 to release the panel IO and its pins (except CS signal).
             *   This flag is only valid for the RGB interface.
             */
    } flags;
} esp_lcd_jd9365_vendor_config_t;

esp_err_t esp_lcd_new_panel_jd9365(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
    esp_lcd_panel_handle_t *ret_panel);

#define JD9365_800_1280_PANEL_60HZ_DPI_CONFIG(px_format) \
    {                                                    \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,     \
        .dpi_clock_freq_mhz = 80,                        \
        .virtual_channel = 0,                            \
        .pixel_format = px_format,                       \
        .num_fbs = 1,                                    \
        .video_timing = {                                \
            .h_size = 800,                               \
            .v_size = 1280,                              \
            .hsync_back_porch = 20,                      \
            .hsync_pulse_width = 20,                     \
            .hsync_front_porch = 40,                     \
            .vsync_back_porch = 10,                      \
            .vsync_pulse_width = 4,                      \
            .vsync_front_porch = 30,                     \
        },                                               \
        .flags.use_dma2d = true,                         \
    }

#ifdef __cplusplus
}
#endif

#endif
