import QtQuick
import QtQuick3D

Node {
    id: node

    // Resources
    PrincipledMaterial {
        id: node0_615686_0_811765_0_929412_0_000000_0_000000_material
        objectName: "0.615686_0.811765_0.929412_0.000000_0.000000"
        baseColor: "#ff9dcfed"
        roughness: 1
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }

    // Nodes:
    Model {
        id: tube
        objectName: "Tube"
        source: "meshes/tube_mesh.mesh"
        materials: [
            node0_615686_0_811765_0_929412_0_000000_0_000000_material,
            node0_615686_0_811765_0_929412_0_000000_0_000000_material,
            node0_615686_0_811765_0_929412_0_000000_0_000000_material,
            node0_615686_0_811765_0_929412_0_000000_0_000000_material
        ]
    }

    // Animations:
}
