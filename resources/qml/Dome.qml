import QtQuick
import QtQuick3D

Node {
    id: node
    
    property string nodeId: ""

    // Resources
    PrincipledMaterial {
        id: domeMaterial
        baseColor: "white"
        metalness: 0.5
        roughness: 0.2
        cullMode: PrincipledMaterial.NoCulling
    }

    // Nodes:
    Model {
        id: dome
        objectName: node.nodeId
        pickable: true
        source: "meshes/dome_mesh.mesh"
        materials: [
            domeMaterial,
            domeMaterial,
            domeMaterial,
            domeMaterial,
            domeMaterial,
            domeMaterial
        ]
    }

    // Animations:
}
