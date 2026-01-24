# Main application executable
qt_add_executable(observatory-monitor
    main.cpp
    Application.cpp
    Application.h
)

qt_add_qml_module(observatory-monitor
    URI ObservatoryMonitor
    VERSION 1.0
    QML_FILES
        ../../resources/qml/main.qml
        ../../resources/qml/Dome.qml
        ../../resources/qml/Pier.qml
        ../../resources/qml/MountAzimuth.qml
        ../../resources/qml/Tube.qml
        ../../resources/qml/CircularGauge.qml
        ../../resources/qml/LevelGauge.qml
    RESOURCES
        ../../resources/models/Dome.gltf
        ../../resources/models/MountAzimuth.gltf
        ../../resources/models/Pier.gltf
        ../../resources/models/Tube.gltf
        ../../resources/qml/meshes/dome_mesh.mesh
        ../../resources/qml/meshes/pier_mesh.mesh
        ../../resources/qml/meshes/mountAzimuth_mesh.mesh
        ../../resources/qml/meshes/tube_mesh.mesh
)

target_link_libraries(observatory-monitor PRIVATE
    observatory-shared
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Quick3D
)

# Install
install(TARGETS observatory-monitor
    RUNTIME DESTINATION bin
)
