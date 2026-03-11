idf_component_register(
    SRCS "src/driver_ledstrip.c"
    INCLUDE_DIRS "include"
    REQUIRES led_strip bsp_manager
)
