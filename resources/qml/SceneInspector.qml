import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: inspectorRoot
    width: 250
    color: app.theme === "Dark" ? "#2d2d2d" : "white"
    border.color: "#444"
    
    property var selectedNode: null
    property var activeNode: {
        // Accessing sceneNodes forces re-evaluation when sceneNodesChanged is emitted
        let dummy = layout ? layout.sceneNodes : [] 
        if (!selectedNode || !layout) return null
        for (let i = 0; i < layout.sceneNodes.length; i++) {
            if (layout.sceneNodes[i].id === selectedNode.id) {
                return layout.sceneNodes[i]
            }
        }
        return null
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15
        
        Label {
            text: qsTr("Scene Node Properties")
            font.bold: true
            font.pixelSize: 16
        }
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ColumnLayout {
                width: parent.width - 20
                spacing: 10
                enabled: activeNode !== null
                
                Label { text: qsTr("Model:") }
                Label {
                    text: activeNode ? activeNode.model : ""
                    font.italic: true
                    color: "gray"
                }

                Label { text: qsTr("Parent Node:") }
                ComboBox {
                    id: parentCombo
                    Layout.fillWidth: true
                    model: {
                        let nodes = ["Scene"]
                        if (layout && activeNode) {
                            for (let i = 0; i < layout.sceneNodes.length; i++) {
                                let n = layout.sceneNodes[i]
                                if (n.id !== activeNode.id) {
                                    nodes.push(n.id)
                                }
                            }
                        }
                        return nodes
                    }
                    currentIndex: {
                        if (!activeNode) return 0
                        let p = (activeNode.parent === "" || !activeNode.parent) ? "Scene" : activeNode.parent
                        let idx = model.indexOf(p)
                        return idx >= 0 ? idx : 0
                    }
                    onActivated: {
                        let val = currentText === "Scene" ? "" : currentText
                        updateNode("parent", val)
                    }
                }
                
                Label { text: qsTr("Position (X, Y, Z):") }
                RowLayout {
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "X"
                        text: activeNode ? activeNode.offset.x.toFixed(2) : "0.00"
                        onEditingFinished: updateNode("offset", Qt.vector3d(parseFloat(text), activeNode.offset.y, activeNode.offset.z))
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Y"
                        text: activeNode ? activeNode.offset.y.toFixed(2) : "0.00"
                        onEditingFinished: updateNode("offset", Qt.vector3d(activeNode.offset.x, parseFloat(text), activeNode.offset.z))
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Z"
                        text: activeNode ? activeNode.offset.z.toFixed(2) : "0.00"
                        onEditingFinished: updateNode("offset", Qt.vector3d(activeNode.offset.x, activeNode.offset.y, parseFloat(text)))
                    }
                }
                
                Label { text: qsTr("Motion Type:") }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["none", "rotation", "linear"]
                    currentIndex: model.indexOf(activeNode && activeNode.motion ? activeNode.motion.type : "none")
                    onActivated: {
                        let m = activeNode.motion || { axis: Qt.vector3d(0,1,0), mapping: {} }
                        m.type = currentText
                        updateNode("motion", m)
                    }
                }

                Label { text: qsTr("Motion Axis (X, Y, Z):") }
                RowLayout {
                    enabled: activeNode && activeNode.motion && activeNode.motion.type !== "none"
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "X"
                        text: (activeNode && activeNode.motion) ? activeNode.motion.axis.x.toFixed(2) : "0.00"
                        onEditingFinished: {
                            let m = activeNode.motion || { mapping: {} }
                            m.axis = Qt.vector3d(parseFloat(text), m.axis.y, m.axis.z)
                            updateNode("motion", m)
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Y"
                        text: (activeNode && activeNode.motion) ? activeNode.motion.axis.y.toFixed(2) : "1.00"
                        onEditingFinished: {
                            let m = activeNode.motion || { mapping: {} }
                            m.axis = Qt.vector3d(m.axis.x, parseFloat(text), m.axis.z)
                            updateNode("motion", m)
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Z"
                        text: (activeNode && activeNode.motion) ? activeNode.motion.axis.z.toFixed(2) : "0.00"
                        onEditingFinished: {
                            let m = activeNode.motion || { mapping: {} }
                            m.axis = Qt.vector3d(m.axis.x, m.axis.y, parseFloat(text))
                            updateNode("motion", m)
                        }
                    }
                }
                
                Label { text: qsTr("Property:") }
                ComboBox {
                    Layout.fillWidth: true
                    model: caps ? caps.allPropertyLinks : []
                    currentIndex: model.indexOf(activeNode && activeNode.motion ? activeNode.motion.property : "")
                    onActivated: {
                        let m = activeNode.motion || { axis: Qt.vector3d(0,1,0), mapping: {} }
                        m.property = currentText
                        updateNode("motion", m)
                    }
                }

                Label { text: qsTr("Mapping (In Min / In Max):") }
                RowLayout {
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "In Min"
                        text: (activeNode && activeNode.motion && activeNode.motion.mapping) ? activeNode.motion.mapping.inMin : "0"
                        onEditingFinished: {
                            let m = activeNode.motion || {}
                            m.mapping = m.mapping || {}
                            m.mapping.inMin = parseFloat(text)
                            updateNode("motion", m)
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "In Max"
                        text: (activeNode && activeNode.motion && activeNode.motion.mapping) ? activeNode.motion.mapping.inMax : "1"
                        onEditingFinished: {
                            let m = activeNode.motion || {}
                            m.mapping = m.mapping || {}
                            m.mapping.inMax = parseFloat(text)
                            updateNode("motion", m)
                        }
                    }
                }

                Label { text: qsTr("Mapping (Out Min / Out Max):") }
                RowLayout {
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Out Min"
                        text: (activeNode && activeNode.motion && activeNode.motion.mapping) ? activeNode.motion.mapping.outMin : "0"
                        onEditingFinished: {
                            let m = activeNode.motion || {}
                            m.mapping = m.mapping || {}
                            m.mapping.outMin = parseFloat(text)
                            updateNode("motion", m)
                        }
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Out Max"
                        text: (activeNode && activeNode.motion && activeNode.motion.mapping) ? activeNode.motion.mapping.outMax : "1"
                        onEditingFinished: {
                            let m = activeNode.motion || {}
                            m.mapping = m.mapping || {}
                            m.mapping.outMax = parseFloat(text)
                            updateNode("motion", m)
                        }
                    }
                }

                Item { Layout.preferredHeight: 10 }

                Button {
                    text: qsTr("Delete Node")
                    Layout.fillWidth: true
                    palette.button: "red"
                    onClicked: {
                        if (activeNode) {
                            layout.removeSceneNode(activeNode.id)
                            selectedNode = null
                        }
                    }
                }
            }
        }
    }
    
    function updateNode(key, value) {
        if (!activeNode) return
        let config = {}
        config[key] = value
        layout.updateSceneNode(activeNode.id, config)
    }
}
