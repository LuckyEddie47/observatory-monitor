# Shared library - common code used by both main app and simulator

add_library(observatory-shared STATIC
    version.cpp
    Config.cpp
    SimulatorConfig.cpp
    Logger.cpp
    MqttClient.cpp
    MqttController.cpp
    MqttController.h
    ControllerPoller.cpp
    ControllerManager.cpp
    ControllerListModel.cpp
    ControllerListModel.h
    ControllerProxy.cpp
    ControllerProxy.h
    AbstractController.h
    Types.h
    CapabilityRegistry.cpp
    CapabilityRegistry.h
    ValueMappingEngine.cpp
    ValueMappingEngine.h
    LayoutConfig.cpp
    LayoutConfig.h
)

target_include_directories(observatory-shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(observatory-shared PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Mqtt
    yaml-cpp
)

target_compile_definitions(observatory-shared PUBLIC
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)
