import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: widgetRoot
    width: 120
    height: 80
    radius: 4
    color: app.theme === "Dark" ? "#2d2d2d" : "white"
    border.color: isSelected ? "blue" : (app.theme === "Dark" ? "#444" : "#ccc")
    border.width: isSelected ? 2 : 1
    
    property string label: "Value"
    property var value: 0
    property string propertyLink: ""
    property var mapping: null
    property bool isSelected: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        
        Label {
            text: widgetRoot.label
            font.pixelSize: 12
            color: app.theme === "Dark" ? "#aaa" : "#666"
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        
        Label {
            text: widgetRoot.value !== undefined ? widgetRoot.value : "---"
            font.pixelSize: 24
            font.bold: true
            color: app.theme === "Dark" ? "white" : "black"
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
