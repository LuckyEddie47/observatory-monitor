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

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            Label {
                text: qsTr("Observatory Monitor")
                font.bold: true
                font.pixelSize: 16
                Layout.leftMargin: 10
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                text: qsTr("Settings")
                onClicked: settingsDialog.open()
            }
        }
    }

    Dialog {
        id: settingsDialog
        title: qsTr("Settings")
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 450
        height: 500
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TabBar {
                id: settingsTabBar
                Layout.fillWidth: true
                TabButton { text: qsTr("GUI") }
                TabButton { text: qsTr("Network") }
                TabButton { text: qsTr("Controllers") }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 15
                currentIndex: settingsTabBar.currentIndex

                // GUI Settings Page
                ScrollView {
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Label {
                            text: qsTr("Appearance")
                            font.bold: true
                        }

                        RowLayout {
                            Label { text: qsTr("Theme:") }
                            ComboBox {
                                id: themeCombo
                                model: ["Dark", "Light", "System"]
                                currentIndex: model.indexOf(app.theme)
                            }
                        }

                        Label {
                            text: qsTr("Visibility")
                            font.bold: true
                            Layout.topMargin: 10
                        }

                        CheckBox {
                            id: showGaugesCheck
                            text: qsTr("Show Visual Gauges")
                            checked: app.showGauges
                        }

                        CheckBox {
                            id: show3DViewCheck
                            text: qsTr("Show 3D Visualization")
                            checked: app.show3DView
                        }

                        RowLayout {
                            Label { text: qsTr("Sidebar Width:") }
                            Slider {
                                id: sidebarWidthSlider
                                from: 200
                                to: 500
                                value: app.sidebarWidth
                            }
                            Label { text: Math.round(sidebarWidthSlider.value) + "px" }
                        }
                    }
                }

                // Network Settings Page
                ScrollView {
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Label {
                            text: qsTr("MQTT Broker")
                            font.bold: true
                        }

                        RowLayout {
                            Label { text: qsTr("Host:"); Layout.preferredWidth: 100 }
                            TextField {
                                id: mqttHostField
                                text: app.mqttHost
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Port:"); Layout.preferredWidth: 100 }
                            SpinBox {
                                id: mqttPortSpin
                                from: 1
                                to: 65535
                                value: app.mqttPort
                                editable: true
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Username:"); Layout.preferredWidth: 100 }
                            TextField {
                                id: mqttUsernameField
                                text: app.mqttUsername
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Password:"); Layout.preferredWidth: 100 }
                            TextField {
                                id: mqttPasswordField
                                text: app.mqttPassword
                                echoMode: TextInput.Password
                                Layout.fillWidth: true
                            }
                        }

                        Label {
                            text: qsTr("MQTT Timing")
                            font.bold: true
                            Layout.topMargin: 10
                        }

                        RowLayout {
                            Label { text: qsTr("Timeout (s):"); Layout.preferredWidth: 100 }
                            SpinBox {
                                id: mqttTimeoutSpin
                                from: 5    // 0.5 * 10
                                to: 300   // 30.0 * 10
                                value: Math.round(app.mqttTimeout * 10)
                                stepSize: 5
                                editable: true
                                
                                property real realValue: value / 10.0

                                textFromValue: function(value, locale) {
                                    return Number(value / 10.0).toLocaleString(locale, 'f', 1)
                                }

                                valueFromText: function(text, locale) {
                                    return Math.round(Number.fromLocaleString(locale, text) * 10)
                                }
                            }
                        }

                        RowLayout {
                            Label { text: qsTr("Reconnect (s):"); Layout.preferredWidth: 100 }
                            SpinBox {
                                id: reconnectIntervalSpin
                                from: 1
                                to: 300
                                value: app.reconnectInterval
                                editable: true
                            }
                        }
                        
                        Label {
                            text: qsTr("Note: Network changes may require application restart.")
                            font.italic: true
                            font.pixelSize: 11
                            color: "orange"
                            Layout.topMargin: 10
                        }
                    }
                }

                // Controllers Settings Page
                ScrollView {
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Label {
                            text: qsTr("Manage Controllers")
                            font.bold: true
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: contentHeight
                            model: controllerModel
                            interactive: false
                            clip: true

                            delegate: RowLayout {
                                width: ListView.view.width
                                spacing: 10
                                
                                CheckBox {
                                    text: name
                                    checked: isEnabled
                                    onToggled: isEnabled = checked
                                }
                                
                                Label {
                                    text: "(" + type + ")"
                                    font.italic: true
                                    color: "gray"
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: status === 2 ? "green" : (status === 3 ? "red" : "orange")
                                }
                            }
                        }

                        Label {
                            text: qsTr("Note: Disabling a controller stops all polling and disconnects from the broker.")
                            font.italic: true
                            font.pixelSize: 11
                            color: "gray"
                            Layout.topMargin: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        onAccepted: {
            // Save GUI settings
            app.theme = themeCombo.currentText
            app.showGauges = showGaugesCheck.checked
            app.show3DView = show3DViewCheck.checked
            app.sidebarWidth = Math.round(sidebarWidthSlider.value)

            // Save Network settings
            app.mqttHost = mqttHostField.text
            app.mqttPort = mqttPortSpin.value
            app.mqttUsername = mqttUsernameField.text
            app.mqttPassword = mqttPasswordField.text
            app.mqttTimeout = mqttTimeoutSpin.realValue
            app.reconnectInterval = reconnectIntervalSpin.value

            app.saveConfig()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: app.sidebarWidth
            color: app.theme === "Dark" ? "#222222" : "#f0f0f0"
            visible: true // Could add a toggle for this too

            ScrollView {
                anchors.fill: parent
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                clip: true

                ColumnLayout {
                    width: parent.width - 20
                    anchors.margins: 10
                    spacing: 15

                    Label {
                        text: qsTr("Controllers")
                        font.pixelSize: 18
                        font.bold: true
                        color: app.theme === "Dark" ? "white" : "black"
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        model: controllerModel
                        interactive: false
                        clip: true

                        delegate: ItemDelegate {
                            width: parent.width
                            background: Rectangle {
                                color: app.theme === "Dark" ? "#333333" : "#e0e0e0"
                                visible: highlighted || hovered
                            }
                            contentItem: RowLayout {
                                spacing: 10
                                ColumnLayout {
                                    Label {
                                        text: name
                                        font.bold: true
                                        color: app.theme === "Dark" ? "white" : "black"
                                    }
                                }
                                Item {
                                    Layout.fillWidth: true
                                }
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
                        color: app.theme === "Dark" ? "#333333" : "white"
                        radius: 5
                        border.color: "#cccccc"
                        visible: observatoryController !== null

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10

                            Label {
                                text: observatoryController ? observatoryController.name : ""
                                font.bold: true
                                color: app.theme === "Dark" ? "white" : "black"
                            }

                            GridLayout {
                                columns: 2
                                Label {
                                    text: "Azimuth:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: observatoryController ? observatoryController.azimuth.toFixed(2) + "°" : "0°"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Altitude:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: observatoryController ? observatoryController.altitude.toFixed(2) + "°" : "0°"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "RA:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: observatoryController ? observatoryController.ra.toFixed(3) : "0"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Dec:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: observatoryController ? observatoryController.dec.toFixed(3) : "0"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Shutter:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: observatoryController ? observatoryController.shutterStatus : "Unknown"
                                    color: observatoryController && observatoryController.shutterStatus === "Error" ? "red" : (app.theme === "Dark" ? "white" : "black")
                                    font.bold: true
                                }
                            }
                        }
                    }

                    ShutterStatus {
                        Layout.alignment: Qt.AlignHCenter
                        status: observatoryController ? observatoryController.shutterStatus : "Unknown"
                        visible: observatoryController !== null
                        color: app.theme === "Dark" ? "#333333" : "white"
                    }

                    // Telescope Data display
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 170
                        color: app.theme === "Dark" ? "#333333" : "white"
                        radius: 5
                        border.color: "#cccccc"
                        visible: telescopeController !== null

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10

                            Label {
                                text: telescopeController ? telescopeController.name : ""
                                font.bold: true
                                color: app.theme === "Dark" ? "white" : "black"
                            }

                            GridLayout {
                                columns: 2
                                Label {
                                    text: "Azimuth:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: telescopeController ? telescopeController.azimuth.toFixed(2) + "°" : "0°"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Altitude:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: telescopeController ? telescopeController.altitude.toFixed(2) + "°" : "0°"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "RA:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: telescopeController ? telescopeController.ra.toFixed(3) : "0"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Dec:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: telescopeController ? telescopeController.dec.toFixed(3) : "0"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }

                                Label {
                                    text: "Side of Pier:"
                                    color: app.theme === "Dark" ? "#bbbbbb" : "#666666"
                                }
                                Label {
                                    text: telescopeController ? telescopeController.sideOfPier : "Unknown"
                                    color: app.theme === "Dark" ? "white" : "black"
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: app.showGauges
                        spacing: 15

                        Label {
                            text: qsTr("Visual Gauges")
                            font.pixelSize: 18
                            font.bold: true
                            Layout.topMargin: 10
                            color: app.theme === "Dark" ? "white" : "black"
                        }

                        CircularGauge {
                            Layout.alignment: Qt.AlignHCenter
                            label: "Dome Azimuth"
                            value: observatoryController ? observatoryController.azimuth : 0
                            width: 180
                            height: 180
                        }

                        CircularGauge {
                            Layout.alignment: Qt.AlignHCenter
                            label: "Mount Azimuth"
                            value: telescopeController ? telescopeController.azimuth : 0
                            width: 180
                            height: 180
                            needleColor: "blue"
                        }

                        LevelGauge {
                            Layout.alignment: Qt.AlignHCenter
                            label: "Altitude"
                            value: telescopeController ? telescopeController.altitude : 0
                            width: 80
                            height: 180
                        }
                    }

                    Item {
                        Layout.preferredHeight: 20
                    }
                }
            }
        }

        // 3D View
        View3D {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: app.show3DView

            environment: SceneEnvironment {
                clearColor: app.theme === "Dark" ? "#111111" : "#eeeeee"
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
                    PhysicalRelationship {
                        type: PhysicalRelationship.Rotation
                        axis: Qt.vector3d(0, 1, 0)
                        value: observatoryController ? -observatoryController.azimuth : 0
                        Dome {
                            id: domeComponent
                        }
                    }

                    // Stationary Pier
                    Pier {
                        id: pierComponent
                    }

                    // Mount Base (Rotating in Azimuth)
                    PhysicalRelationship {
                        type: PhysicalRelationship.Rotation
                        axis: Qt.vector3d(0, 1, 0)
                        value: telescopeController ? -telescopeController.azimuth : 0

                        MountAzimuth {
                            id: mountAzimuthComponent

                            // Telescope Tube (Rotating in Altitude)
                            PhysicalRelationship {
                                type: PhysicalRelationship.Rotation
                                axis: Qt.vector3d(1, 0, 0)
                                value: telescopeController ? -telescopeController.altitude : 0
                                Tube {
                                    id: tubeComponent
                                }
                            }
                        }
                    }
                }
            }

            Node {
                id: cameraPivot
                position: Qt.vector3d(0, 0, 0)

                Node {
                    id: cameraOrbit
                    eulerRotation.x: -25

                    PerspectiveCamera {
                        id: camera
                        position: Qt.vector3d(0, 0, 600)
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property real lastX: 0
                property real lastY: 0

                onPressed: (mouse) => {
                    lastX = mouse.x
                    lastY = mouse.y
                }

                onPositionChanged: (mouse) => {
                    let dx = mouse.x - lastX
                    let dy = mouse.y - lastY
                    
                    if (mouse.buttons & Qt.LeftButton) {
                        // Pan: move the pivot point in the plane of the camera
                        let panSpeed = camera.z / 1000.0
                        let move = Qt.vector3d(-dx * panSpeed, dy * panSpeed, 0)
                        cameraPivot.position = cameraPivot.position.plus(cameraOrbit.mapDirectionToScene(move))
                    } else if (mouse.buttons & Qt.RightButton) {
                        // Rotate: orbit around the pivot
                        cameraOrbit.eulerRotation.y -= dx * 0.2
                        cameraOrbit.eulerRotation.x = Math.max(-90, Math.min(20, cameraOrbit.eulerRotation.x - dy * 0.2))
                    }
                    
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }

            WheelHandler {
                onWheel: (event) => {
                    let zoomSpeed = camera.z * 0.1
                    let newZ = camera.z - (event.angleDelta.y / 120.0) * zoomSpeed
                    camera.z = Math.max(100, Math.min(4000, newZ))
                }
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
            Item {
                Layout.fillWidth: true
            }
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
