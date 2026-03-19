idf_component_register(
    SRCS
        "src/board_ate_p4.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_gpio
        esp_driver_uart
        esp_driver_i2c
        esp_eth
        esp_common
        esp_hw_support
)
