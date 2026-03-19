idf_component_register(
    SRCS
        "src/system_instrument_service.c"
    INCLUDE_DIRS "include"
    REQUIRES
        system_comm_mgr
        system_uart_link
)
