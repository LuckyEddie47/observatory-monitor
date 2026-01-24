---
description: Project Roadmap and Technical Context
alwaysApply: true
---

# Project Roadmap & Context

## Historical Context
The project was developed in stages (Phases 1-6) as a Qt6/C++ application to monitor observatory equipment (telescopes, domes) via MQTT. It targets the **LX200 protocol** superset used by **OCS (Observatory Control System)** and **OnStepX**.

### Completed Phases
- **Phase 1-3**: Core structure, YAML configuration (`Config` class), and custom `Logger`.
- **Phase 4**: `MqttClient` with a command queue and response timeout handling.
- **Phase 5**: `mqtt-simulator` for hardware-free testing.
- **Phase 6**: `ControllerManager` for multi-controller orchestration and status polling.

## Current State Analysis
- **Main Logic**: Heavy procedural logic in `main.cpp` used for testing Phase 6 needs to be refactored into an `Application` or `Core` class.
- **Architecture**: `ControllerManager` is tightly coupled to `MqttClient`. An abstraction layer (e.g., `AbstractController`) is needed to support non-MQTT sources or future expansions.
- **QML Integration**: The project is currently console-based. Transitioning to a GUI requires exposing C++ models to QML.
- **Robustness**: The 1:1 command-response matching in `MqttClient` is a potential point of failure for asynchronous status updates.

## Proposed Roadmap

### 1. Refactoring & Core Stability (High Priority)
- [x] Move `main.cpp` logic into a dedicated `Application` class.
- [x] Implement an `AbstractController` interface to decouple protocol logic from management logic.
- [x] Improve `MqttClient` response parsing to handle unsolicited updates.
- [x] Standardize cross-platform path handling for logs and config.

### 2. GUI Foundation
- [x] Initialize `QQmlApplicationEngine` in the new `Application` class.
- [x] Create `ControllerListModel` (inheriting `QAbstractListModel`) to expose real-time status to QML.

### 3. 3D Visualization (Qt Quick 3D)
- [x] Set up the 3D scene with basic primitives.
- [x] Implement the **Physical Relationship System** for Azimuth/Altitude rotations.
- [x] Integrate Onshape-exported glTF models.

### 4. Graphical Status Panels
- [x] Design and implement custom QML gauges (circular, level, text).
- [ ] Create a system for user-configurable panel backgrounds and layouts.

## Technical Debt to Watch
- Raw `new` calls in `MqttClient` (prefer `std::unique_ptr` where appropriate).
- Permissions/paths for `/var/log` on non-Linux systems.
