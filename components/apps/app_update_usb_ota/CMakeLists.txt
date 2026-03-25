idf_component_register(
    SRCS "src/app_update_usb_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES system_usb esp_event
    PRIV_REQUIRES app_ui
)
