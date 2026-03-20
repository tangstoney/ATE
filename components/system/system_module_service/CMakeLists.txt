idf_component_register(
    SRCS
        "src/system_module_service.c"
    INCLUDE_DIRS "include"
    REQUIRES
        system_comm_mgr
        driver_i2c_master
        system_protocol
        system_fault
        system_event
        freertos
)
