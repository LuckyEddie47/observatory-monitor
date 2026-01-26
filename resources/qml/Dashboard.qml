import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: dashboardRoot
    
    property var selectedWidget: null
    
    Rectangle {
        anchors.fill: parent
        color: layout.backgroundColor !== "transparent" ? layout.backgroundColor : (app.theme === "Dark" ? "#1a1a1a" : "#f5f5f5")
        
        Image {
            anchors.fill: parent
            source: layout.backgroundSource
            fillMode: Image.PreserveAspectCrop
            visible: layout.backgroundSource !== ""
        }

        // Grid background for editor mode
        Canvas {
            anchors.fill: parent
            visible: app.editorMode
            opacity: 0.1
            onPaint: {
                var ctx = getContext("2d");
                ctx.strokeStyle = app.theme === "Dark" ? "white" : "black";
                ctx.lineWidth = 1;
                ctx.beginPath();
                for (var x = 0; x < width; x += 20) {
                    ctx.moveTo(x, 0);
                    ctx.lineTo(x, height);
                }
                for (var y = 0; y < height; y += 20) {
                    ctx.moveTo(0, y);
                    ctx.lineTo(width, y);
                }
                ctx.stroke();
            }
        }
    }

    // Dynamic Widgets
    Repeater {
        model: layout ? layout.widgets : []
        
        Loader {
            id: widgetLoader
            x: modelData ? modelData.x : 0
            y: modelData ? modelData.y : 0
            source: modelData ? "widgets/" + modelData.type + "Widget.qml" : ""
            
            property var widgetData: modelData
            
            onLoaded: {
                if (!widgetData) return;
                item.label = widgetData.label || ""
                item.propertyLink = widgetData.propertyLink || ""
                item.mapping = widgetData.mapping || ({})
                item.isSelected = (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.id === widgetData.id)
                
                // Set up data binding
                if (widgetData.propertyLink) {
                    setupBinding(item, widgetData.propertyLink, widgetData.mapping)
                }
            }

            DragHandler {
                enabled: app.editorMode
                onActiveChanged: {
                    if (!active) {
                        layout.updateWidget(widgetData.id, {
                            "x": widgetLoader.x,
                            "y": widgetLoader.y
                        })
                    }
                }
            }

            TapHandler {
                enabled: app.editorMode
                onTapped: dashboardRoot.selectedWidget = widgetData
            }

            // Selection Highlight
            Rectangle {
                anchors.fill: parent
                color: "blue"
                opacity: 0.1
                visible: app.editorMode && (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.id === widgetData.id)
                border.color: "blue"
                border.width: 2
            }
            
            Connections {
                target: dashboardRoot
                function onSelectedWidgetChanged() {
                    if (widgetLoader.item && widgetData) {
                        widgetLoader.item.isSelected = (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.id === widgetData.id)
                    }
                }
            }
        }
    }

    // Editor Controls
    RowLayout {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10
        
        Button {
            text: app.editorMode ? qsTr("Exit Editor") : qsTr("Edit Dashboard")
            onClicked: {
                app.editorMode = !app.editorMode
                if (!app.editorMode) {
                    dashboardRoot.selectedWidget = null
                    app.saveLayout()
                }
            }
        }
        
        Button {
            visible: app.editorMode
            text: qsTr("Add Widget")
            onClicked: libraryDrawer.open()
        }
    }

    // Property Inspector (Right Panel)
    Rectangle {
        id: inspectorPanel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 250
        visible: app.editorMode
        color: app.theme === "Dark" ? "#2d2d2d" : "white"
        border.color: "#444"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 15
            
            Label {
                text: dashboardRoot.selectedWidget ? qsTr("Widget Properties") : qsTr("Dashboard Settings")
                font.bold: true
                font.pixelSize: 16
            }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: parent.width
                    spacing: 10
                    
                    // Dashboard Global Settings
                    ColumnLayout {
                        visible: !dashboardRoot.selectedWidget
                        width: parent.width
                        spacing: 10

                        Label { text: qsTr("Background Color:") }
                        RowLayout {
                            TextField {
                                id: bgColorField
                                Layout.fillWidth: true
                                text: layout.backgroundColor
                                placeholderText: "#RRGGBB or name"
                                onEditingFinished: layout.backgroundColor = text
                            }
                            Rectangle {
                                width: 20
                                height: 20
                                color: layout.backgroundColor
                                border.color: "gray"
                            }
                        }

                        Label { text: qsTr("Background Image (URL/Path):") }
                        TextField {
                            Layout.fillWidth: true
                            text: layout.backgroundSource
                            placeholderText: "file:///path/to/image.jpg"
                            onEditingFinished: layout.backgroundSource = text
                        }
                        
                        Button {
                            text: qsTr("Clear Background")
                            Layout.fillWidth: true
                            onClicked: {
                                layout.backgroundSource = ""
                                layout.backgroundColor = "transparent"
                                bgColorField.text = "transparent"
                            }
                        }
                    }

                    // Widget specific settings
                    ColumnLayout {
                        visible: !!dashboardRoot.selectedWidget
                        width: parent.width
                        spacing: 10

                        Label { text: qsTr("Label:") }
                        TextField {
                            Layout.fillWidth: true
                            text: dashboardRoot.selectedWidget ? dashboardRoot.selectedWidget.label : ""
                            onTextChanged: {
                                if (dashboardRoot.selectedWidget && layout) {
                                    layout.updateWidget(dashboardRoot.selectedWidget.id, {"label": text})
                                }
                            }
                        }
                        
                        Label { text: qsTr("Property:") }
                        ComboBox {
                            id: propertyCombo
                            Layout.fillWidth: true
                            model: caps ? caps.allPropertyLinks : []
                            currentIndex: model.indexOf(dashboardRoot.selectedWidget ? dashboardRoot.selectedWidget.property : "")
                            onActivated: {
                                if (dashboardRoot.selectedWidget && layout) {
                                    layout.updateWidget(dashboardRoot.selectedWidget.id, {"property": currentText})
                                }
                            }
                        }

                        Label { 
                            text: qsTr("Mapping:")
                            font.bold: true
                            Layout.topMargin: 10
                        }

                        RowLayout {
                            Label { text: "In Min:"; Layout.preferredWidth: 50 }
                            TextField {
                                Layout.fillWidth: true
                                text: (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.mapping) ? dashboardRoot.selectedWidget.mapping.inMin : "0"
                                onEditingFinished: {
                                    if (dashboardRoot.selectedWidget && layout) {
                                        let m = dashboardRoot.selectedWidget.mapping || {}
                                        m.inMin = parseFloat(text)
                                        m.type = "linear"
                                        layout.updateWidget(dashboardRoot.selectedWidget.id, {"mapping": m})
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Label { text: "In Max:"; Layout.preferredWidth: 50 }
                            TextField {
                                Layout.fillWidth: true
                                text: (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.mapping) ? dashboardRoot.selectedWidget.mapping.inMax : "1"
                                onEditingFinished: {
                                    if (dashboardRoot.selectedWidget && layout) {
                                        let m = dashboardRoot.selectedWidget.mapping || {}
                                        m.inMax = parseFloat(text)
                                        m.type = "linear"
                                        layout.updateWidget(dashboardRoot.selectedWidget.id, {"mapping": m})
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Label { text: "Out Min:"; Layout.preferredWidth: 50 }
                            TextField {
                                Layout.fillWidth: true
                                text: (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.mapping) ? dashboardRoot.selectedWidget.mapping.outMin : "0"
                                onEditingFinished: {
                                    if (dashboardRoot.selectedWidget && layout) {
                                        let m = dashboardRoot.selectedWidget.mapping || {}
                                        m.outMin = parseFloat(text)
                                        m.type = "linear"
                                        layout.updateWidget(dashboardRoot.selectedWidget.id, {"mapping": m})
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Label { text: "Out Max:"; Layout.preferredWidth: 50 }
                            TextField {
                                Layout.fillWidth: true
                                text: (dashboardRoot.selectedWidget && dashboardRoot.selectedWidget.mapping) ? dashboardRoot.selectedWidget.mapping.outMax : "1"
                                onEditingFinished: {
                                    if (dashboardRoot.selectedWidget && layout) {
                                        let m = dashboardRoot.selectedWidget.mapping || {}
                                        m.outMax = parseFloat(text)
                                        m.type = "linear"
                                        layout.updateWidget(dashboardRoot.selectedWidget.id, {"mapping": m})
                                    }
                                }
                            }
                        }
                        
                        Button {
                            text: qsTr("Delete Widget")
                            Layout.fillWidth: true
                            palette.button: "red"
                            onClicked: {
                                if (dashboardRoot.selectedWidget && layout) {
                                    layout.removeWidget(dashboardRoot.selectedWidget.id)
                                    dashboardRoot.selectedWidget = null
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Widget Library (Drawer)
    Drawer {
        id: libraryDrawer
        width: 200
        height: parent.height
        edge: Qt.LeftEdge
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Label {
                text: qsTr("Widget Library")
                font.bold: true
            }
            
            ListModel {
                id: widgetLibraryModel
                ListElement { name: "Numeric"; type: "Numeric" }
                ListElement { name: "Analog Gauge"; type: "Analog" }
                ListElement { name: "Binary Indicator"; type: "Binary" }
            }
            
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: widgetLibraryModel
                delegate: ItemDelegate {
                    width: parent.width
                    text: name
                    onClicked: {
                        var id = "widget_" + Date.now()
                        layout.addWidget({
                            "id": id,
                            "type": type,
                            "label": name,
                            "x": 50,
                            "y": 50,
                            "property": ""
                        })
                        libraryDrawer.close()
                    }
                }
            }
        }
    }

    function setupBinding(widget, link, mapping) {
        if (!link) return;
        
        var parts = link.split('.');
        if (parts.length < 2) return;
        
        var controllerName = parts[0];
        var propertyName = parts[1];
        
        var controller = app.getController(controllerName);
        if (!controller) return;

        var updateValue = function() {
            var rawValue = controller.getProperty(propertyName);
            widget.value = valueMappingEngine.mapValue(rawValue, mapping);
        };

        updateValue();

        // Use a persistent connection mechanism or handle cleanup
        controller.propertyChanged.connect(function(name, value) {
            if (name === propertyName) {
                widget.value = valueMappingEngine.mapValue(value, mapping);
            }
        });
    }
}
