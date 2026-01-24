import QtQuick
import QtQuick3D

Node {
    id: node

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
        objectName: "Tube"
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
