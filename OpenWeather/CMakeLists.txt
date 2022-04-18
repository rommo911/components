idf_component_register(SRCS "src/openWeather.cpp"
                    INCLUDE_DIRS "include"	
                     PRIV_REQUIRES webclient nlohmann_json Arduino_lib nvs_tools
                    )