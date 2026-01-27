import QtQuick
import QtQuick3D

Node {
    id: root
    
    property var sceneNodes: []
    
    // Shared materials to reduce draw calls and memory
    property PrincipledMaterial defaultMaterial: PrincipledMaterial {
        baseColor: "silver"
        metalness: 0.5
        roughness: 0.1
    }

    property PrincipledMaterial xMaterial: PrincipledMaterial { baseColor: "red"; lighting: PrincipledMaterial.NoLighting }
    property PrincipledMaterial yMaterial: PrincipledMaterial { baseColor: "green"; lighting: PrincipledMaterial.NoLighting }
    property PrincipledMaterial zMaterial: PrincipledMaterial { baseColor: "blue"; lighting: PrincipledMaterial.NoLighting }
    property PrincipledMaterial selectionMaterial: PrincipledMaterial { baseColor: "yellow"; lighting: PrincipledMaterial.NoLighting }

    // Pre-calculate children map for O(1) lookup during tree traversal
    property var childrenMap: {
        let map = {};
        if (sceneNodes) {
            for (let i = 0; i < sceneNodes.length; i++) {
                let node = sceneNodes[i];
                let p = node.parent || "";
                if (!map[p]) map[p] = [];
                map[p].push(node);
            }
        }
        return map;
    }
    
    // Root nodes (those with empty or missing parent)
    Instantiator {
        model: root.childrenMap[""] || []
        
        delegate: Loader {
            source: "SceneNode.qml"
            onLoaded: {
                if (item) {
                    item.parent = root
                    item.nodeData = Qt.binding(function() { return modelData })
                    item.childrenMap = Qt.binding(function() { return root.childrenMap })
                    item.sharedMaterials = Qt.binding(function() {
                        return {
                            "default": root.defaultMaterial,
                            "x": root.xMaterial,
                            "y": root.yMaterial,
                            "z": root.zMaterial,
                            "selection": root.selectionMaterial
                        }
                    })
                }
            }
        }
    }
}
