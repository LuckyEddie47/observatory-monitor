# Main application executable

add_executable(observatory-monitor
    main.cpp
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
