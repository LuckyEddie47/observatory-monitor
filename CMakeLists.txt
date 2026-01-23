cmake_minimum_required(VERSION 3.28)

project(ObservatoryMonitor VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 with specific path
set(Qt6_DIR "/home/ed/Qt/6.8.3/gcc_64/lib/cmake/Qt6")

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Qml
    Quick
    QuickControls2
    Quick3D
    Mqtt
)

# Qt Policies
qt_policy(SET QTP0001 NEW)

# Find yaml-cpp
find_package(yaml-cpp REQUIRED)

# Add subdirectories
add_subdirectory(src/shared)
add_subdirectory(src/main)
add_subdirectory(src/simulator)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Summary
message(STATUS "==============================================")
message(STATUS "Observatory Monitor v${PROJECT_VERSION}")
message(STATUS "==============================================")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================")
