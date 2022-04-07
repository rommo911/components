idf_component_register(SRCS "src/openWeather.cpp"
                    INCLUDE_DIRS "include"	
                     REQUIRES Arduino_lib nlohmann_json nvs_tools
                    )