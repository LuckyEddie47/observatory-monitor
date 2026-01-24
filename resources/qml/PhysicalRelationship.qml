import QtQuick
import QtQuick3D

Node {
    id: root
    
    enum Type {
        Rotation,
        Linear
    }
    
    property int type: PhysicalRelationship.Rotation
    property vector3d axis: Qt.vector3d(0, 1, 0) // Default Y-axis
    property real value: 0 // Degrees for rotation, Meters for linear
    
    // For Rotation: apply to eulerRotation
    // For Linear: apply to position
    
    onValueChanged: updateTransform()
    onAxisChanged: updateTransform()
    onTypeChanged: updateTransform()
    
    function updateTransform() {
        if (root.type === PhysicalRelationship.Rotation) {
            // Apply rotation around specified axis
            root.eulerRotation = Qt.vector3d(axis.x * value, axis.y * value, axis.z * value)
            root.position = Qt.vector3d(0, 0, 0)
        } else {
            // Apply translation along specified axis
            root.position = Qt.vector3d(axis.x * value, axis.y * value, axis.z * value)
            root.eulerRotation = Qt.vector3d(0, 0, 0)
        }
    }
    
    Component.onCompleted: updateTransform()
}
