idf_component_register(SRCS "src/FreeRTOS.cpp"
					"src/task_tools.cpp"
                    "src/semphr.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES debug_tools
					)