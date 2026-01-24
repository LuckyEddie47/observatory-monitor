import QtQuick
import QtQuick3D

Node {
    id: node

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
        objectName: "Dome"
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
