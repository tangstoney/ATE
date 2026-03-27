idf_component_register(
    SRCS "src/app_ate_console.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        app_ui
        app_module_runtime
        app_update_usb_ota
        app_ate_gateway
        esp_event
)
