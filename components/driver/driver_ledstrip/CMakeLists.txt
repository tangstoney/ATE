idf_component_register(
    SRCS "src/driver_ledstrip.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES
        board
        espressif__led_strip
)
