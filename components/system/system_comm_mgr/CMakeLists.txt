idf_component_register(
    SRCS
        "src/system_comm_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES
        driver_i2c_master
        system_uart_link
        freertos
)
