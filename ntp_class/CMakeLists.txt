idf_component_register(SRCS "src/ntp_class.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES debug_tools lwip nvs_tools system_tools)
