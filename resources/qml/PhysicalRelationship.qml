import QtQuick
import QtQuick3D

Node {
    id: root
    
    enum Type {
        Rotation,
        Linear,
        None
    }
    
    property int type: PhysicalRelationship.Rotation
    property vector3d axis: Qt.vector3d(0, 1, 0) // Default Y-axis
    property real value: 0 // Degrees for rotation, Meters for linear
    
    property string propertyLink: ""
    property var mapping: ({})
    
    property var targetController: null
    property string targetPropertyName: ""
    property string targetCommand: ""

    onPropertyLinkChanged: {
        if (!propertyLink) {
            targetController = null;
            targetPropertyName = "";
            targetCommand = "";
            return;
        }
        
        var parts = propertyLink.split('.');
        if (parts.length < 2) return;
        
        var controllerName = parts[0];
        targetPropertyName = parts[1];
        
        var propDef = caps.getProperty(controllerName, targetPropertyName);
        targetCommand = propDef ? propDef.command : targetPropertyName;
        
        targetController = app.getController(controllerName);
        updateValue();
    }
    
    onMappingChanged: updateValue()

    function updateValue() {
        if (targetController && targetCommand) {
            var rawValue = targetController.getProperty(targetCommand);
            root.value = valueMappingEngine.mapValue(rawValue, mapping);
        }
    }

    Connections {
        target: root.targetController
        ignoreUnknownSignals: true
        function onPropertyChanged(name, val) {
            if (name === root.targetCommand || name === root.targetPropertyName) {
                root.value = valueMappingEngine.mapValue(val, root.mapping);
            }
        }
    }
    
    onValueChanged: {
        if (Math.abs(value - lastAppliedValue) > 0.0001) {
            updateTransform();
            lastAppliedValue = value;
        }
    }
    
    property real lastAppliedValue: -999999
    
    onAxisChanged: updateTransform()
    onTypeChanged: updateTransform()
    
    function updateTransform() {
        // Cache vectors to avoid repeated allocations
        let ax = axis.x;
        let ay = axis.y;
        let az = axis.z;
        let v = value;

        if (root.type === PhysicalRelationship.Rotation) {
            root.eulerRotation = Qt.vector3d(ax * v, ay * v, az * v)
            if (root.position.x !== 0 || root.position.y !== 0 || root.position.z !== 0)
                root.position = Qt.vector3d(0, 0, 0)
        } else if (root.type === PhysicalRelationship.Linear) {
            root.position = Qt.vector3d(ax * v, ay * v, az * v)
            if (root.eulerRotation.x !== 0 || root.eulerRotation.y !== 0 || root.eulerRotation.z !== 0)
                root.eulerRotation = Qt.vector3d(0, 0, 0)
        } else {
            if (root.position.x !== 0 || root.position.y !== 0 || root.position.z !== 0)
                root.position = Qt.vector3d(0, 0, 0)
            if (root.eulerRotation.x !== 0 || root.eulerRotation.y !== 0 || root.eulerRotation.z !== 0)
                root.eulerRotation = Qt.vector3d(0, 0, 0)
        }
    }
    
    Component.onCompleted: {
        updateTransform()
        updateValue()
    }
}
