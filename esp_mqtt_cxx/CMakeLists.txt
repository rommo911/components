idf_build_get_property(target IDF_TARGET)

idf_component_register(SRC_DIRS "src"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE "priv_include"
                    REQUIRES mqtt cxx
                    )
target_compile_options(${COMPONENT_LIB} PRIVATE "-std=gnu++17")
