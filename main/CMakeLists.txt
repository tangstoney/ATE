idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES board system_device_manager app_ate_console nvs_flash
)
