---
description: Repository Information Overview
alwaysApply: true
---

# Observatory Monitor Information

## Repository Summary
Observatory Monitor is a cross-platform Qt6/C++ application designed for monitoring astronomical observatory equipment via MQTT. It provides a 3D visualization of the observatory components (Dome, Pier, Mount, Tube) and synchronizes their movements with real-time telemetry received via MQTT topics.

## Repository Structure
The project is organized into a shared library and two executable components:
- **src/shared**: A static library containing core logic, MQTT client, controller management, and configuration handling.
- **src/main**: The primary GUI application using Qt Quick and Qt Quick 3D.
- **src/simulator**: A mock MQTT client utility for simulating observatory equipment telemetry.
- **resources**: Contains QML files for the UI and 3D models (GLTF/Mesh).
- **tests**: Unit tests for configuration and logging components.
- **config**: Example configuration files and local log storage.

### Main Repository Components
- **observatory-shared**: Common utilities, MQTT communication, and data models.
- **observatory-monitor**: The graphical user interface for monitoring.
- **mqtt-simulator**: A command-line tool to simulate equipment behavior for testing.

## Projects

### observatory-shared (Shared Library)
**Configuration File**: `src/shared/CMakeLists.txt`

#### Language & Runtime
**Language**: C++  
**Version**: C++17  
**Build System**: CMake  
**Package Manager**: System-level (yaml-cpp) and Qt6 SDK

#### Dependencies
**Main Dependencies**:
- Qt6 (Core, Mqtt)
- yaml-cpp (v0.8.0+)

### observatory-monitor (Main Application)
**Configuration File**: `src/main/CMakeLists.txt`

#### Language & Runtime
**Language**: C++, QML  
**Version**: C++17, Qt 6.8.3  
**Build System**: CMake

#### Dependencies
**Main Dependencies**:
- observatory-shared
- Qt6 (Qml, Quick, QuickControls2, Quick3D)

#### Build & Installation
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Main Files & Resources
- **Entry Point**: `src/main/main.cpp`
- **Main QML**: `resources/qml/main.qml`
- **Configuration**: `~/.config/observatory-monitor/config.yaml`

### mqtt-simulator (Simulator Utility)
**Configuration File**: `src/simulator/CMakeLists.txt`

#### Language & Runtime
**Language**: C++  
**Version**: C++17  
**Build System**: CMake

#### Dependencies
**Main Dependencies**:
- observatory-shared
- Qt6 (Core, Mqtt)

#### Build & Installation
```bash
# Built as part of the main project build
./build/src/simulator/mqtt-simulator --config config/simulator.yaml
```

#### Main Files & Resources
- **Entry Point**: `src/simulator/simulator_main.cpp`
- **Configuration**: `config/simulator.yaml`

## Testing

**Framework**: Qt Test
**Test Location**: `tests/`
**Naming Convention**: `test_*.cpp`
**Configuration**: `tests/CMakeLists.txt`

**Run Command**:
```bash
cd build
ctest --output-on-failure
```
