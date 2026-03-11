idf_component_register(
    SRCS "src/system_init.c"
    INCLUDE_DIRS "include"
    REQUIRES system_log system_config system_storage system_event_bus system_network system_display
)
