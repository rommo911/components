idf_component_register(SRCS "src/spiffs_tools.cpp"
"src/File_2.cpp"
							"src/sdCard.cpp"
                            "src/storage_tools.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES spi_flash debug_tools esp_littlefs nvs_flash fatfs
					)
