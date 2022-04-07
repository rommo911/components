idf_component_register(SRCS "src/nvs_tools.cpp" 
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES  debug_tools nvs_flash 
					)
