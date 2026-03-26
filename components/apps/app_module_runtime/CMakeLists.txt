idf_component_register(
    SRCS "src/app_module_runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_event
        freertos
        system_module
)
