idf_component_register(
    SRCS "priv_include/encoder.c" "src/rotary.cpp"
    INCLUDE_DIRS "include" "priv_include"
    PRIV_REQUIRES system_tools idf_event_cxx
)
