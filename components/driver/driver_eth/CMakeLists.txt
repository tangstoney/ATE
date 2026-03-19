idf_component_register(
    SRCS "driver_eth.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_eth esp_driver_gpio board
)
