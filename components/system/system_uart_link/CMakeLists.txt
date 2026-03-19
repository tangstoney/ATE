idf_component_register(
    SRCS
        "src/system_uart_link.c"
    INCLUDE_DIRS "include"
    REQUIRES
        driver_uart_port
)
