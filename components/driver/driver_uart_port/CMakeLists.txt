idf_component_register(
    SRCS
        "src/driver_uart_port.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_uart
)
