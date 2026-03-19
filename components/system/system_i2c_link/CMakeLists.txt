idf_component_register(
    SRCS
        "src/system_i2c_link.c"
    INCLUDE_DIRS "include"
    REQUIRES
        driver_i2c_master
)
