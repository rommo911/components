idf_component_register(SRCS "src/wifi_directory.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "." 
                    REQUIRES storage_tools nvs_tools debug_tools json)
