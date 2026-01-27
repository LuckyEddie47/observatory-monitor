# Copilot Instructions for Observatory Monitor

## Project Overview
- **Observatory Monitor** is a cross-platform Qt6/C++ application for monitoring astronomical observatory equipment via MQTT.
- Main components:
  - **Main GUI** (`src/main/`): QML-based dashboard, entry point is `main.cpp` and `Application.h/cpp`.
  - **Shared Library** (`src/shared/`): Core logic, configuration, MQTT, logging, controller management.
  - **Simulator** (`src/simulator/`): Mock MQTT client for testing and development.
  - **Resources** (`resources/qml/`, `resources/models/`): QML UI and 3D models.
  - **Config** (`config/`): YAML configuration files for runtime settings.

## Build & Test Workflow
- **Build:** Use CMake (see `CMakeLists.txt`). Qt6 and yaml-cpp are required. Example:
  - `cmake -B build -S .`
  - `cmake --build build`
- **Run:** Main binary is built from `src/main/`. Simulator is built from `src/simulator/`.
- **Test:** Unit tests are in `tests/` and use Qt Test. Run with CTest:
  - `ctest --test-dir build/tests`

## Key Architectural Patterns
- **Configuration:** Centralized in `Config` class (`src/shared/Config.h/cpp`), loaded from YAML files in `config/`.
- **Controllers:** Managed via `ControllerManager`, exposed to QML via `ControllerProxy` and `ControllerListModel`.
- **MQTT Communication:** Encapsulated in `MqttClient` (`src/shared/MqttClient.h/cpp`). Handles command queueing, responses, and reconnection logic.
- **Logging:** Singleton `Logger` (`src/shared/Logger.h/cpp`), supports file rotation and debug toggling.
- **QML Integration:** Properties and signals in `Application` class are exposed to QML for UI binding.

## Project-Specific Conventions
- **All configuration and layout changes should be persisted via `saveConfig()` and `saveLayout()` methods in `Application`.
- **MQTT topics and command patterns are set via configuration and managed in code, not hardcoded in QML.
- **Debug logging can be toggled at runtime using the Logger singleton.
- **QML files reference C++ backend via registered types and properties (see `main.qml`, `Application.h`).

## Integration Points
- **External dependencies:** Qt6 (Core, Qml, Quick, QuickControls2, Quick3D, Mqtt), yaml-cpp, Mosquitto MQTT broker.
- **Simulator:** Use `src/simulator/simulator_main.cpp` to mock MQTT traffic for development/testing.

## Example: Adding a New Controller
1. Define controller config in YAML (`config/config.yaml`).
2. Extend controller logic in C++ (`src/shared/ControllerManager.cpp`, `ControllerProxy.cpp`).
3. Expose new controller to QML via `ControllerListModel`.
4. Update QML UI as needed (`resources/qml/`).

## References
- Main entry: `src/main/main.cpp`, `src/main/Application.h/cpp`
- Configuration: `src/shared/Config.h/cpp`, `config/config.yaml`
- MQTT: `src/shared/MqttClient.h/cpp`
- Logging: `src/shared/Logger.h/cpp`
- QML UI: `resources/qml/main.qml`

---
If any section is unclear or missing, please provide feedback to improve these instructions.
