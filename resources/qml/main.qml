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
    
    // Validation error banner
    Rectangle {
        id: validationBanner
        width: parent.width
        height: (layout && !layout.isValid) ? 40 : 0
        color: "#e74c3c"
        visible: height > 0
        z: 999
        anchors.top: parent.header.bottom
        
        Behavior on height { NumberAnimation { duration: 200 } }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            Label {
                text: "⚠ " + (layout ? layout.validationError : "")
                color: "white"
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            Button {
                text: "Reset to Defaults"
                flat: true
                palette.buttonText: "white"
                onClicked: {
                    layout.setDefaults()
                    layout.validate(caps, "")
                }
            }
        }
    }
    
    property var selectedSceneNode: null

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
                text: app.showDashboard ? qsTr("3D View") : qsTr("Dashboard")
                onClicked: app.showDashboard = !app.showDashboard
            }
            ToolButton {
                visible: !app.showDashboard
                text: app.editorMode ? qsTr("Exit Editor") : qsTr("Edit Scene")
                onClicked: {
                    app.editorMode = !app.editorMode
                    if (!app.editorMode) {
                        selectedSceneNode = null
                        app.saveLayout()
                    }
                }
            }
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

                        RowLayout {
                            Label { text: qsTr("Sidebar Position:") }
                            ComboBox {
                                id: sidebarPositionCombo
                                model: ["Left", "Right"]
                                currentIndex: model.indexOf(app.sidebarPosition)
                            }
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
            app.sidebarPosition = sidebarPositionCombo.currentText

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
        layoutDirection: app.sidebarPosition === "Right" ? Qt.RightToLeft : Qt.LeftToRight

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

        // Main Content Area
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: app.showDashboard ? 1 : 0

            // 3D View
            View3D {
                id: view3D
                visible: app.show3DView && !app.showDashboard

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
                    scale: Qt.vector3d(100, 100, 100)

                    SceneComposer {
                        sceneNodes: layout ? layout.sceneNodes : []
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
                property bool moved: false

                onPressed: (mouse) => {
                    lastX = mouse.x
                    lastY = mouse.y
                    moved = false
                }

                onClicked: (mouse) => {
                    if (!moved && app.editorMode && mouse.button === Qt.LeftButton) {
                        let result = view3D.pick(mouse.x, mouse.y)
                        if (result.objectHit) {
                            let nodeId = result.objectHit.objectName
                            if (nodeId) {
                                // Find node data in layout model
                                for (let i = 0; i < layout.sceneNodes.length; i++) {
                                    if (layout.sceneNodes[i].id === nodeId) {
                                        selectedSceneNode = layout.sceneNodes[i]
                                        break
                                    }
                                }
                            }
                        } else {
                            selectedSceneNode = null
                        }
                    }
                }

                onPositionChanged: (mouse) => {
                    let dx = mouse.x - lastX
                    let dy = mouse.y - lastY
                    
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) moved = true
                    
                    if (mouse.buttons & Qt.LeftButton) {
                        if (app.editorMode && selectedSceneNode) {
                            // Object dragging logic
                            // Move object in the plane parallel to the camera
                            // Divide by 100.0 to account for observatoryNode scale
                            let moveScale = (camera.z / 1000.0) / 100.0
                            let move = Qt.vector3d(dx * moveScale, -dy * moveScale, 0)
                            let rotatedMove = cameraOrbit.mapDirectionToScene(move)
                            
                            let currentOffset = selectedSceneNode.offset
                            let newOffset = Qt.vector3d(currentOffset.x + rotatedMove.x,
                                                       currentOffset.y + rotatedMove.y,
                                                       currentOffset.z + rotatedMove.z)
                            
                            layout.updateSceneNode(selectedSceneNode.id, { "offset": newOffset })
                            
                            // Update our local copy so the next frame's dx/dy is applied to the new position
                            selectedSceneNode.offset = newOffset 
                        } else {
                            // Pan: move the pivot point in the plane of the camera
                            let panSpeed = camera.z / 1000.0
                            let move = Qt.vector3d(-dx * panSpeed, dy * panSpeed, 0)
                            cameraPivot.position = cameraPivot.position.plus(cameraOrbit.mapDirectionToScene(move))
                        }
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

        Dashboard {
                id: customDashboard
                visible: app.showDashboard
            }
        }
    }

    Rectangle {
        id: sceneEditorPanel
        width: 300
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: app.editorMode && !app.showDashboard
        color: app.theme === "Dark" ? "#222222" : "#f0f0f0"
        border.color: "#444"
        z: 10 // Ensure it's above the 3D view
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Scene Editor")
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Exit")
                    onClicked: {
                        app.editorMode = false
                        selectedSceneNode = null
                        app.saveLayout()
                    }
                }
            }
            
            Label { text: qsTr("Scene Nodes") }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                model: layout ? layout.sceneNodes : []
                clip: true
                delegate: ItemDelegate {
                    width: parent.width
                    text: modelData.id + " (" + modelData.model + ")"
                    highlighted: selectedSceneNode && selectedSceneNode.id === modelData.id
                    onClicked: selectedSceneNode = modelData
                }
            }

            Label { text: qsTr("Add Asset") }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: assetCombo
                    Layout.fillWidth: true
                    model: ["#Cube", "#Sphere", "#Cylinder", "Dome.qml", "Pier.qml", "MountAzimuth.qml", "Tube.qml"]
                }
                Button {
                    text: "+"
                    onClicked: {
                        let id = assetCombo.currentText.toLowerCase().replace(".qml", "") + "_" + Date.now()
                        let model = assetCombo.currentText
                        layout.addSceneNode({
                            "id": id,
                            "model": model,
                            "parent": "",
                            "offset": Qt.vector3d(0, 0, 0),
                            "motion": { "type": "none", "axis": Qt.vector3d(0,1,0), "property": "" }
                        })
                    }
                }
            }
            
            SceneInspector {
                Layout.fillWidth: true
                Layout.fillHeight: true
                selectedNode: selectedSceneNode
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
