idf_component_register(
    SRCS
        "src/system_router.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_event
        freertos
)
