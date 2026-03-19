idf_component_register(
    SRCS
        "src/system_fault.c"
    INCLUDE_DIRS "include"
    REQUIRES
        system_event
        freertos
)
