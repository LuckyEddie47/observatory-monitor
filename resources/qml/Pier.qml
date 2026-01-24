import QtQuick
import QtQuick3D

Node {
    id: node

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
        objectName: "Pier"
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
