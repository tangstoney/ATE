idf_component_register(
    SRCS
        "src/driver_display.c"
        "src/driver_display_lcd.c"
        "src/driver_display_touch.c"
        "src/esp_lcd_jd9365_waveshare.c"
    INCLUDE_DIRS "include"
    REQUIRES
        board
        esp_lvgl_adapter
        lvgl
        esp_lcd
        esp_lcd_touch
        esp_lcd_touch_gt911
        esp_driver_gpio
        esp_driver_i2c
        esp_hw_support
)
