idf_component_register(SRCS "src/mqtt_class.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES mqtt debug_tools
					nvs_tools
                    system_tools
					EMBED_TXTFILES mqtt_client.crt
					mqtt_client.key
					)
