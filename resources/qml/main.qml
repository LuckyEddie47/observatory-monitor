import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

ApplicationWindow {
    width: 1024
    height: 768
    visible: true
    title: qsTr("Observatory Monitor")

    property var observatoryController: app.getController("Observatory")
    property var telescopeController: app.getController("Telescope")

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 250
            color: "#f0f0f0"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    text: qsTr("Controllers")
                    font.pixelSize: 18
                    font.bold: true
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: controllerModel
                    clip: true

                    delegate: ItemDelegate {
                        width: parent.width
                        contentItem: RowLayout {
                            spacing: 10
                            ColumnLayout {
                                Label {
                                    text: name
                                    font.bold: true
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: status === 2 ? "green" : (status === 3 ? "red" : "orange")
                            }
                        }
                    }
                }

                // Data display
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    color: "white"
                    radius: 5
                    border.color: "#cccccc"
                    visible: observatoryController !== null

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        
                        Label {
                            text: observatoryController ? observatoryController.name : ""
                            font.bold: true
                        }
                        
                        GridLayout {
                            columns: 2
                            Label { text: "Azimuth:" }
                            Label { text: observatoryController ? observatoryController.azimuth.toFixed(2) + "°" : "0°" }
                            
                            Label { text: "Altitude:" }
                            Label { text: observatoryController ? observatoryController.altitude.toFixed(2) + "°" : "0°" }

                            Label { text: "RA:" }
                            Label { text: observatoryController ? observatoryController.ra.toFixed(2) : "0" }

                            Label { text: "Dec:" }
                            Label { text: observatoryController ? observatoryController.dec.toFixed(2) : "0" }
                        }
                    }
                }

                // Telescope Data display
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    color: "white"
                    radius: 5
                    border.color: "#cccccc"
                    visible: telescopeController !== null

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        
                        Label {
                            text: telescopeController ? telescopeController.name : ""
                            font.bold: true
                        }
                        
                        GridLayout {
                            columns: 2
                            Label { text: "Azimuth:" }
                            Label { text: telescopeController ? telescopeController.azimuth.toFixed(2) + "°" : "0°" }
                            
                            Label { text: "Altitude:" }
                            Label { text: telescopeController ? telescopeController.altitude.toFixed(2) + "°" : "0°" }

                            Label { text: "RA:" }
                            Label { text: telescopeController ? telescopeController.ra.toFixed(2) : "0" }

                            Label { text: "Dec:" }
                            Label { text: telescopeController ? telescopeController.dec.toFixed(2) : "0" }
                        }
                    }
                }
            }
        }

        // 3D View
        View3D {
            Layout.fillWidth: true
            Layout.fillHeight: true

            environment: SceneEnvironment {
                clearColor: "#111111"
                backgroundMode: SceneEnvironment.Color
            }

            // Fallback indicator if 3D scene has issues
            Text {
                anchors.centerIn: parent
                text: "3D Visualization Active"
                color: "white"
                visible: false // Only for debugging if needed
            }

            Node {
                id: sceneRoot

                DirectionalLight {
                    eulerRotation.x: -30
                    eulerRotation.y: -30
                }

                // Simple Ground Plane
                Model {
                    position: Qt.vector3d(0, -100, 0)
                    scale: Qt.vector3d(10, 0.1, 10)
                    source: "#Cube"
                    materials: [
                        DefaultMaterial {
                            diffuseColor: "gray"
                        }
                    ]
                }

                // Placeholder for the observatory/telescope
                Node {
                    id: observatoryNode
                    scale: Qt.vector3d(100, 100, 100) // Onshape usually exports in mm, View3D uses meters

                    // Rotating Dome
                    Dome {
                        id: domeComponent
                        eulerRotation.y: observatoryController ? -observatoryController.azimuth : 0
                    }

                    // Stationary Pier
                    Pier {
                        id: pierComponent
                    }

                    // Mount Base (Rotating in Azimuth)
                    MountAzimuth {
                        id: mountAzimuthComponent
                        eulerRotation.y: telescopeController ? -telescopeController.azimuth : 0

                        // Telescope Tube (Rotating in Altitude)
                        // Assuming the Tube pivot is already aligned in the component
                        Tube {
                            id: tubeComponent
                            eulerRotation.x: telescopeController ? -telescopeController.altitude : 0
                        }
                    }
                }
            }

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 300, 600)
                eulerRotation.x: -25
            }

            OrbitCameraController {
                camera: camera
                origin: sceneRoot
            }
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            Label {
                text: qsTr("System Status: ") + app.systemStatus
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Label {
                text: Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm:ss")
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: parent.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm:ss")
                }
            }
        }
    }
}
