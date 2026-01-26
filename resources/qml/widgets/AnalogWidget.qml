import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: widgetRoot
    width: 200
    height: 220
    radius: 4
    color: app.theme === "Dark" ? "#2d2d2d" : "white"
    border.color: isSelected ? "blue" : (app.theme === "Dark" ? "#444" : "#ccc")
    border.width: isSelected ? 2 : 1
    
    property string label: "Analog"
    property var value: 0
    property string propertyLink: ""
    property var mapping: null
    property bool isSelected: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        
        CircularGauge {
            Layout.alignment: Qt.AlignHCenter
            label: widgetRoot.label
            value: widgetRoot.value !== undefined ? widgetRoot.value : 0
            width: 180
            height: 180
        }
    }
}
