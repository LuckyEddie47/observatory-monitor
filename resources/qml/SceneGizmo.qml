import QtQuick
import QtQuick3D

Node {
    id: root
    
    property string nodeId: ""
    property var sharedMaterials: ({})
    
    // Selection highlight
    Model {
        visible: selectedSceneNode && selectedSceneNode.id === nodeId
        source: "#Cube"
        scale: Qt.vector3d(0.011, 0.011, 0.011)
        opacity: 0.1
        materials: [ sharedMaterials.selection || null ]
    }

    // Y Axis (Green)
    Model {
        source: "#Cylinder"
        scale: Qt.vector3d(0.001, 0.01, 0.001)
        position: Qt.vector3d(0, 1.25, 0)
        materials: [ sharedMaterials.y || null ]
    }
    Model {
        source: "#Cone"
        scale: Qt.vector3d(0.001, 0.002, 0.001)
        position: Qt.vector3d(0, 2.5, 0)
        materials: [ sharedMaterials.y || null ]
    }

    // X Axis (Red)
    Model {
        source: "#Cylinder"
        eulerRotation.z: -90
        scale: Qt.vector3d(0.001, 0.01, 0.001)
        position: Qt.vector3d(1.25, 0, 0)
        materials: [ sharedMaterials.x || null ]
    }
    Model {
        source: "#Cone"
        eulerRotation.z: -90
        scale: Qt.vector3d(0.001, 0.002, 0.001)
        position: Qt.vector3d(2.5, 0, 0)
        materials: [ sharedMaterials.x || null ]
    }

    // Z Axis (Blue)
    Model {
        source: "#Cylinder"
        eulerRotation.x: 90
        scale: Qt.vector3d(0.001, 0.01, 0.001)
        position: Qt.vector3d(0, 0, 1.25)
        materials: [ sharedMaterials.z || null ]
    }
    Model {
        source: "#Cone"
        eulerRotation.x: 90
        scale: Qt.vector3d(0.001, 0.002, 0.001)
        position: Qt.vector3d(0, 0, 2.5)
        materials: [ sharedMaterials.z || null ]
    }
}
