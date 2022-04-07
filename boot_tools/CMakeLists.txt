idf_component_register(SRCS 
                    "src/boot_cfg.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES debug_tools
                    app_update 
                    nvs_tools
                    system_tools
					)