# Simulator executable

add_executable(mqtt-simulator
    simulator_main.cpp
)

target_link_libraries(mqtt-simulator PRIVATE
    observatory-shared
    Qt6::Core
    Qt6::Mqtt
)

# Install
install(TARGETS mqtt-simulator
    RUNTIME DESTINATION bin
)
