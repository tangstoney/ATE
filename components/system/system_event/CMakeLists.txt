idf_component_register(
    SRCS
        "src/system_event.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_event
        freertos
)
