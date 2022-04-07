idf_component_register(SRCS "src/buzzer.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES driver
                    debug_tools
                    system_tools
                    rtos
                    )