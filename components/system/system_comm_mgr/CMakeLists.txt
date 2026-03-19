idf_component_register(
    SRCS
        "src/system_comm_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES
        system_i2c_link
        system_uart_link
        freertos
)
