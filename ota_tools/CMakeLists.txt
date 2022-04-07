idf_component_register(SRCS "src/_ota.cpp"
					"src/ota_server.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES debug_tools
					esp_https_ota app_update
                    rtos
					nvs_tools system_tools)