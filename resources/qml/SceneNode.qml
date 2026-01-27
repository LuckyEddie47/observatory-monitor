import QtQuick
import QtQuick3D

Node {
    id: root
    
    property var nodeData: ({})
    property var childrenMap: ({})
    property var sharedMaterials: ({})
    
    // Static offset from parent
    position: (nodeData && nodeData.offset) ? nodeData.offset : Qt.vector3d(0, 0, 0)
    
    PhysicalRelationship {
        id: motionNode
        
        type: {
            if (!nodeData || !nodeData.motion) return PhysicalRelationship.None;
            if (nodeData.motion.type === "rotation") return PhysicalRelationship.Rotation;
            if (nodeData.motion.type === "linear") return PhysicalRelationship.Linear;
            return PhysicalRelationship.None;
        }
        
        axis: (nodeData && nodeData.motion && nodeData.motion.axis) ? nodeData.motion.axis : Qt.vector3d(0, 1, 0)
        propertyLink: (nodeData && nodeData.motion && nodeData.motion.property) ? nodeData.motion.property : ""
        mapping: (nodeData && nodeData.motion && nodeData.motion.mapping) ? nodeData.motion.mapping : ({})
        
        // Handle QML models
        Loader {
            id: qmlModelLoader
            source: {
                if (!nodeData || !nodeData.model) return "";
                let m = nodeData.model;
                if (m.endsWith(".qml")) return m;
                // Auto-append .qml if it's a known component name and not a primitive
                if (!m.startsWith("#") && !m.endsWith(".mesh") && !m.endsWith(".glb") && !m.endsWith(".gltf") && !m.endsWith(".obj")) {
                    return m + ".qml";
                }
                return "";
            }
            onLoaded: {
                if (item) {
                    item.parent = motionNode
                    if (item.hasOwnProperty("nodeId")) item.nodeId = nodeData.id
                }
            }
        }

        // Handle direct Mesh/3D models
        Model {
            id: directModel
            pickable: true
            objectName: (nodeData && nodeData.id) ? nodeData.id : ""
            visible: source !== ""
            source: {
                if (!nodeData || !nodeData.model) return "";
                let m = nodeData.model;
                if (m.endsWith(".qml")) return "";
                if (m.startsWith("#")) return m; // Built-in primitives
                if (m.endsWith(".mesh") || m.endsWith(".glb") || m.endsWith(".gltf") || m.endsWith(".obj")) return m;
                return "";
            }
            materials: [ sharedMaterials.default || null ]
        }
        
        // Origin visualization (Gizmo) - Only instantiated in Editor Mode
        Loader {
            id: gizmoLoader
            active: app.editorMode
            source: "SceneGizmo.qml"
            onLoaded: {
                if (item) {
                    item.parent = motionNode
                    item.nodeId = Qt.binding(function() { return nodeData ? nodeData.id : "" })
                    item.sharedMaterials = Qt.binding(function() { return root.sharedMaterials })
                }
            }
        }

        // Recursively instantiate children
        Instantiator {
            model: (nodeData && nodeData.id) ? (childrenMap[nodeData.id] || []) : []
            
            delegate: Loader {
                source: "SceneNode.qml"
                onLoaded: {
                    if (item) {
                        item.parent = motionNode
                        item.nodeData = Qt.binding(function() { return modelData })
                        item.childrenMap = Qt.binding(function() { return root.childrenMap })
                        item.sharedMaterials = Qt.binding(function() { return root.sharedMaterials })
                    }
                }
            }
        }
    }
}
