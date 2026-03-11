idf_component_register(
    SRCS "system_network.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event
    PRIV_REQUIRES driver_eth esp_netif freertos
)
