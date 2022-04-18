idf_component_register(SRC_DIRS "src"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES
                    idf_event_cxx
					debug_tools
					nlohmann_json
                    PRIV_REQUIRES  bootloader_support app_update libsodium nvs_tools)