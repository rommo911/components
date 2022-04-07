idf_component_register(SRCS "src/ambient_sensor.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES debug_tools hass system_tools rtos )