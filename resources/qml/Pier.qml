import QtQuick
import QtQuick3D

Node {
    id: node

    // Resources
    PrincipledMaterial {
        id: node0_701961_0_701961_0_701961_0_000000_0_000000_material
        objectName: "0.701961_0.701961_0.701961_0.000000_0.000000"
        baseColor: "#ffb3b3b3"
        roughness: 1
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }

    // Nodes:
    Model {
        id: pier
        objectName: "Pier"
        source: "meshes/pier_mesh.mesh"
        materials: [
            node0_701961_0_701961_0_701961_0_000000_0_000000_material,
            node0_701961_0_701961_0_701961_0_000000_0_000000_material,
            node0_701961_0_701961_0_701961_0_000000_0_000000_material,
            node0_701961_0_701961_0_701961_0_000000_0_000000_material
        ]
    }

    // Animations:
}
