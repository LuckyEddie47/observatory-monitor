import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: widgetRoot
    width: 140
    height: 60
    radius: 4
    color: app.theme === "Dark" ? "#2d2d2d" : "white"
    border.color: isSelected ? "blue" : (app.theme === "Dark" ? "#444" : "#ccc")
    border.width: isSelected ? 2 : 1
    
    property string label: "Status"
    property var value: false
    property string propertyLink: ""
    property var mapping: null
    property bool isSelected: false

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        Rectangle {
            width: 20
            height: 20
            radius: 10
            color: widgetRoot.value ? "green" : "red"
            
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        
        Label {
            text: widgetRoot.label
            font.bold: true
            color: app.theme === "Dark" ? "white" : "black"
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
}
