idf_component_register(
    SRCS "encoder.c" "rotary.cpp"
    INCLUDE_DIRS .
    PRIV_REQUIRES system_tools rtos
)
