idf_component_register(SRCS "src/Button2.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES
					system_tools arduino-esp32 nlohmann_json )
