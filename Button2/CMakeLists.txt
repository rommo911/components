idf_component_register(SRCS "src/Button2.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES
					system_tools Arduino nlohmann_json )
