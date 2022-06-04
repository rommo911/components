idf_component_register(SRCS "src/openWeather.cpp"
                    INCLUDE_DIRS "include"	
                     PRIV_REQUIRES nlohmann_json arduino-esp32 nvs_tools
                    )