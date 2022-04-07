idf_component_register(SRCS 
					"src/utilities.cpp"
					"src/esp32-hal-psram.c"
                    "src/esp_exception.cpp"
                    "src/esp_event_api.cpp"
                    "src/esp_event_cxx.cpp"
                    "src/esp_timer_cxx.cpp"
                    "src/config.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES
					debug_tools
					nlohmann_json
                    PRIV_REQUIRES rtos bootloader_support app_update libsodium nvs_tools
 )