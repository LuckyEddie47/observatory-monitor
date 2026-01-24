import QtQuick
import QtQuick3D

Node {
    id: node

    // Resources
    PrincipledMaterial {
        id: mountMaterial
        baseColor: "#1a1a2e"
        metalness: 0.6
        roughness: 0.3
        cullMode: PrincipledMaterial.NoCulling
    }

    // Nodes:
    Model {
        id: mountAzimuth
        objectName: "MountAzimuth"
        source: "meshes/mountAzimuth_mesh.mesh"
        materials: [
            mountMaterial,
            mountMaterial,
            mountMaterial,
            mountMaterial
        ]
    }

    // Animations:
}
