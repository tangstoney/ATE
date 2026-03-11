#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_DISPLAY_COLOR_BLACK  = 0x0000,
    SYSTEM_DISPLAY_COLOR_WHITE  = 0xFFFF,
    SYSTEM_DISPLAY_COLOR_RED    = 0xF800,
    SYSTEM_DISPLAY_COLOR_GREEN  = 0x07E0,
    SYSTEM_DISPLAY_COLOR_BLUE   = 0x001F,
    SYSTEM_DISPLAY_COLOR_GRAY   = 0x8410,
} system_display_color_t;

typedef enum {
    SYSTEM_DISPLAY_TEST_SOLID_BLACK = 0,
    SYSTEM_DISPLAY_TEST_SOLID_WHITE,
    SYSTEM_DISPLAY_TEST_SOLID_RED,
    SYSTEM_DISPLAY_TEST_SOLID_GREEN,
    SYSTEM_DISPLAY_TEST_SOLID_BLUE,
    SYSTEM_DISPLAY_TEST_SOLID_GRAY,
    SYSTEM_DISPLAY_TEST_COLOR_BARS,
    SYSTEM_DISPLAY_TEST_GRADIENT,
    SYSTEM_DISPLAY_TEST_CHECKERBOARD,
} system_display_test_pattern_t;

esp_err_t system_display_test_run(system_display_test_pattern_t pattern);

// Hardware pattern: on/off only (no pattern type exposed)
esp_err_t system_display_test_hw_pattern_start(void);
esp_err_t system_display_test_hw_pattern_stop(void);

#ifdef __cplusplus
}
#endif
