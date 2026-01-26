import QtQuick
import QtQuick3D

Node {
    id: node
    
    property string nodeId: ""

    // Resources
    PrincipledMaterial {
        id: pierMaterial
        baseColor: "#333333"
        roughness: 0.8
        cullMode: PrincipledMaterial.NoCulling
    }

    // Nodes:
    Model {
        id: pier
        objectName: node.nodeId
        pickable: true
        source: "meshes/pier_mesh.mesh"
        materials: [
            pierMaterial,
            pierMaterial,
            pierMaterial,
            pierMaterial
        ]
    }

    // Animations:
}
