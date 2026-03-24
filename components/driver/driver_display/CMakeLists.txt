idf_component_register(
    SRCS
        "src/driver_display.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_lcd
    PRIV_REQUIRES
        driver_lcd_panel
        driver_touch_gt911
        esp_lcd_touch
        esp_lvgl_adapter
        lvgl
)
