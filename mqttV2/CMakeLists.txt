idf_component_register(SRC_DIRS "src"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES esp_mqtt_cxx
					nvs_tools
					EMBED_TXTFILES mqtt_client2.crt
					mqtt_client2.key
					)
 