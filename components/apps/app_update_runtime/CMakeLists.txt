idf_component_register(
    SRCS "src/app_update_runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update_usb_ota app_update_network_ota
)
