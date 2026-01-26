import QtQuick
import QtQuick3D

Node {
    id: node
    
    property string nodeId: ""

    // Resources
    PrincipledMaterial {
        id: tubeMaterial
        baseColor: "#1a1a2e"
        metalness: 0.6
        roughness: 0.3
        cullMode: PrincipledMaterial.NoCulling
    }

    // Nodes:
    Model {
        id: tube
        objectName: node.nodeId
        pickable: true
        source: "meshes/tube_mesh.mesh"
        materials: [
            tubeMaterial,
            tubeMaterial,
            tubeMaterial,
            tubeMaterial
        ]
    }

    // Animations:
}
