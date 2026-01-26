# Roadmap: Modular Dashboard & Scene Builder

This document outlines the implementation plan for transitioning the Observatory Monitor into a customizable dashboard and 3D scene builder.

## Core Architectural Principles
- **YAML-Based**: All layouts and scenes are persisted in human-readable YAML.
- **Parent-Child Hierarchy**: 3D models are positioned relative to their parent's origin with X, Y, Z offsets.
- **1-DOF Kinematics**: Each 3D node supports exactly one movement (linear or rotational) driven by a property.
- **Property Mapping**: Supports linear scaling (Min/Max mapping) and binary state indicators.
- **Extensible Property Discovery**: Uses user-editable capability lists per controller type.

---

## Phase 1: Foundation & Data Architecture
- **Dynamic Asset Manager**:
    - Backend to index embedded assets (`qrc://`) and local filesystem assets (e.g., `models/`).
    - Support for GLTF/OBJ formats.
- **Property Capability Registry**:
    - Implement a system to store and load lists of "known properties" for each controller type (Observatory, Telescope, etc.).
    - Allow users to manually expand these lists via YAML configuration.
- **Value Mapping Engine**:
    - Implement logic for linear transformations: `output = ((input - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin`.
    - Support binary mapping for status indicators (e.g., "0/1" or "Open/Closed" to color/state).

## Phase 2: Configuration & YAML Schema
- **Unified Layout Schema**:
    - Design a `layout.yaml` that separates 2D UI elements from 3D scene nodes.
    - **2D Entry**: `{type: "Gauge", position: [x, y], property: "Telescope.altitude", mapping: {min: 0, max: 90}}`.
    - **3D Entry**: `{model: "tube.glb", parent: "mount", offset: [0, 10, 0], motion: {type: "rotation", axis: [1, 0, 0], property: "Telescope.altitude"}}`.

## Phase 3: 2D Dashboard Builder (UI Editor)
- **Editor Mode**: Global toggle to enable the layout canvas.
- **Component Library**: Sidebar containing draggable widgets:
    - **Analog**: Circular Gauges, Level Gauges, Sliders.
    - **Digital**: Labels, Text readouts.
    - **Binary**: Status LEDs, Switch indicators.
- **Property Inspector**: Sidebar UI to:
    - Bind a widget to a property selected from the Capability Registry.
    - Configure labels and mapping/scaling parameters.

## Phase 4: 3D Scene Composer
- **Recursive Scene Loader**:
    - Refactor `View3D` to dynamically instantiate nodes from the YAML tree.
    - Maintain strict parent-child transform hierarchies.
- **Kinematic Component**:
    - A specialized QML component that handles the 1-DOF movement logic locally for each node.
- **Origin & Offset Visualization**:
    - Helper gizmos (X/Y/Z axes) visible in Editor Mode to assist with placing child models at origin offsets.

## Phase 5: Refinement & UX
- **Manual Edit Validation**:
    - Add checks to ensure manual YAML changes don't reference non-existent properties or missing files.
- **Property Discovery Expansion**:
    - Ensure the architecture can eventually support dynamic command-based capability polling (e.g., `get_capabilities`).
- **Performance Optimization**:
    - Ensure smooth performance on integrated GPUs by optimizing 3D node updates.
