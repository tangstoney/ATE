#include "system_display_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "lvgl.h"

#include "system_display.h"
#include "system_display_internal.h"

static const char *TAG = "system_display_test";

static lv_obj_t *s_canvas;
static lv_color_t *s_canvas_buf;
static uint32_t s_canvas_buf_size;
static uint16_t s_canvas_w;
static uint16_t s_canvas_h;

static lv_obj_t *ensure_canvas(void)
{
    if (s_canvas) {
        return s_canvas;
    }

    uint16_t w = 0;
    uint16_t h = 0;
    if (system_display_get_resolution(&w, &h) != ESP_OK) {
        return NULL;
    }

    s_canvas_w = w;
    s_canvas_h = h;
    s_canvas_buf_size = (uint32_t)w * h * sizeof(lv_color_t);
    s_canvas_buf = lv_malloc(s_canvas_buf_size);
    if (!s_canvas_buf) {
        return NULL;
    }

    s_canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(s_canvas, s_canvas_buf, w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(s_canvas, w, h);
    lv_obj_set_pos(s_canvas, 0, 0);

    return s_canvas;
}

static void fill_color(lv_color_t color)
{
    lv_canvas_fill_bg(s_canvas, color, LV_OPA_COVER);
}

static void draw_color_bars(void)
{
    const lv_color_t colors[8] = {
        lv_color_make(0, 0, 0),
        lv_color_make(255, 255, 255),
        lv_color_make(255, 0, 0),
        lv_color_make(0, 255, 0),
        lv_color_make(0, 0, 255),
        lv_color_make(0, 255, 255),
        lv_color_make(255, 0, 255),
        lv_color_make(255, 255, 0),
    };

    uint16_t bar_w = s_canvas_w / 8;
    for (uint16_t x = 0; x < s_canvas_w; ++x) {
        uint8_t idx = x / bar_w;
        if (idx > 7) {
            idx = 7;
        }
        for (uint16_t y = 0; y < s_canvas_h; ++y) {
            lv_canvas_set_px(s_canvas, x, y, colors[idx], LV_OPA_COVER);
        }
    }
}

static void draw_gradient(void)
{
    for (uint16_t x = 0; x < s_canvas_w; ++x) {
        uint8_t v = (uint32_t)x * 255 / (s_canvas_w - 1);
        lv_color_t color = lv_color_make(v, v, v);
        for (uint16_t y = 0; y < s_canvas_h; ++y) {
            lv_canvas_set_px(s_canvas, x, y, color, LV_OPA_COVER);
        }
    }
}

static void draw_checkerboard(void)
{
    const uint16_t cell = 40;
    for (uint16_t y = 0; y < s_canvas_h; ++y) {
        for (uint16_t x = 0; x < s_canvas_w; ++x) {
            bool even = ((x / cell) + (y / cell)) % 2 == 0;
            lv_color_t color = even ? lv_color_make(255, 255, 255) : lv_color_make(0, 0, 0);
            lv_canvas_set_px(s_canvas, x, y, color, LV_OPA_COVER);
        }
    }
}

esp_err_t system_display_test_run(system_display_test_pattern_t pattern)
{
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");
    ESP_RETURN_ON_ERROR(system_display_lock(), TAG, "lock failed");

    lv_obj_t *canvas = ensure_canvas();
    if (!canvas) {
        system_display_unlock();
        return ESP_ERR_NO_MEM;
    }

    switch (pattern) {
    case SYSTEM_DISPLAY_TEST_SOLID_BLACK:
        fill_color(lv_color_make(0, 0, 0));
        break;
    case SYSTEM_DISPLAY_TEST_SOLID_WHITE:
        fill_color(lv_color_make(255, 255, 255));
        break;
    case SYSTEM_DISPLAY_TEST_SOLID_RED:
        fill_color(lv_color_make(255, 0, 0));
        break;
    case SYSTEM_DISPLAY_TEST_SOLID_GREEN:
        fill_color(lv_color_make(0, 255, 0));
        break;
    case SYSTEM_DISPLAY_TEST_SOLID_BLUE:
        fill_color(lv_color_make(0, 0, 255));
        break;
    case SYSTEM_DISPLAY_TEST_SOLID_GRAY:
        fill_color(lv_color_make(128, 128, 128));
        break;
    case SYSTEM_DISPLAY_TEST_COLOR_BARS:
        draw_color_bars();
        break;
    case SYSTEM_DISPLAY_TEST_GRADIENT:
        draw_gradient();
        break;
    case SYSTEM_DISPLAY_TEST_CHECKERBOARD:
        draw_checkerboard();
        break;
    default:
        fill_color(lv_color_make(0, 0, 0));
        break;
    }

    system_display_unlock();
    return ESP_OK;
}

esp_err_t system_display_test_hw_pattern_start(void)
{
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;
    (void)io;
    ESP_RETURN_ON_ERROR(system_display_get_panel_handles(&panel, &io), TAG, "get panel failed");

    return esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_BAR_HORIZONTAL);
}

esp_err_t system_display_test_hw_pattern_stop(void)
{
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;
    (void)io;
    ESP_RETURN_ON_ERROR(system_display_get_panel_handles(&panel, &io), TAG, "get panel failed");

    return esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_NONE);
}
