idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES system_device_manager app_ate_console nvs_flash
)
