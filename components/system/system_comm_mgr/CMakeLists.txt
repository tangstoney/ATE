idf_component_register(
    SRCS
        "src/system_comm_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_i2c
        system_uart_link
        freertos
)
