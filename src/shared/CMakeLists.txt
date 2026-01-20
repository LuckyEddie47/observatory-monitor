# Shared library - common code used by both main app and simulator

add_library(observatory-shared STATIC
    version.cpp
    Config.cpp
    SimulatorConfig.cpp
)

target_include_directories(observatory-shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(observatory-shared PUBLIC
    Qt6::Core
    Qt6::Mqtt
    yaml-cpp
)

target_compile_definitions(observatory-shared PUBLIC
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)
