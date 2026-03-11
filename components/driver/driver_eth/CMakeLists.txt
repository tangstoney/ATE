idf_component_register(
    SRCS "driver_eth.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_eth bsp_manager
)
