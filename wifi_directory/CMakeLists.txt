idf_component_register(SRCS "src/wifi_directory.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "." 
                    PRIV_REQUIRES debug_tools json Arduino_lib )
