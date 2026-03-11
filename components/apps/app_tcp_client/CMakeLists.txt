idf_component_register(
    SRCS "app_tcp_client.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES system_network freertos esp_event
)
