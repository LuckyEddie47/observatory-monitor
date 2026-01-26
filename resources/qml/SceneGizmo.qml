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
        scale: Qt.vector3d(1.1, 1.1, 1.1)
        opacity: 0.1
        materials: [ sharedMaterials.selection || null ]
    }

    // Y Axis (Green)
    Model {
        source: "#Cylinder"
        scale: Qt.vector3d(0.002, 0.2, 0.002)
        position: Qt.vector3d(0, 0.1, 0)
        materials: [ sharedMaterials.y || null ]
    }
    Model {
        source: "#Cone"
        scale: Qt.vector3d(0.008, 0.015, 0.008)
        position: Qt.vector3d(0, 0.2075, 0)
        materials: [ sharedMaterials.y || null ]
    }

    // X Axis (Red)
    Model {
        source: "#Cylinder"
        eulerRotation.z: -90
        scale: Qt.vector3d(0.002, 0.2, 0.002)
        position: Qt.vector3d(0.1, 0, 0)
        materials: [ sharedMaterials.x || null ]
    }
    Model {
        source: "#Cone"
        eulerRotation.z: -90
        scale: Qt.vector3d(0.008, 0.015, 0.008)
        position: Qt.vector3d(0.2075, 0, 0)
        materials: [ sharedMaterials.x || null ]
    }

    // Z Axis (Blue)
    Model {
        source: "#Cylinder"
        eulerRotation.x: 90
        scale: Qt.vector3d(0.002, 0.2, 0.002)
        position: Qt.vector3d(0, 0, 0.1)
        materials: [ sharedMaterials.z || null ]
    }
    Model {
        source: "#Cone"
        eulerRotation.x: 90
        scale: Qt.vector3d(0.008, 0.015, 0.008)
        position: Qt.vector3d(0, 0, 0.2075)
        materials: [ sharedMaterials.z || null ]
    }
}
