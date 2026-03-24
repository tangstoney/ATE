idf_component_register(
    SRCS "src/app_ate_console.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        app_ui_view
        app_ui_controller
        app_ui_state_binding
        app_update_usb_ota
        app_ate_gateway
)
