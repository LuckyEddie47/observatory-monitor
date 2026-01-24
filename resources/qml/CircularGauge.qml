import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property double value: 0
    property double minValue: 0
    property double maxValue: 360
    property string unit: "°"
    property string label: ""
    property color needleColor: "red"
    property color scaleColor: "#333333"

    width: 200
    height: 200

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#f8f8f8"
        border.color: "#dddddd"
        border.width: 2
    }

    // Scale markings
    Repeater {
        model: 12
        delegate: Item {
            width: root.width
            height: root.height
            rotation: index * 30

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 5
                anchors.horizontalCenter: parent.horizontalCenter
                width: 2
                height: 10
                color: root.scaleColor
            }

            Text {
                anchors.top: parent.top
                anchors.topMargin: 18
                anchors.horizontalCenter: parent.horizontalCenter
                text: (index * 30).toString()
                font.pixelSize: 10
                color: root.scaleColor
                rotation: -parent.rotation
            }
        }
    }

    // Needle
    Item {
        id: needle
        width: parent.width
        height: parent.height
        rotation: (root.value - root.minValue) / (root.maxValue - root.minValue) * 360

        Behavior on rotation {
            NumberAnimation {
                duration: 500
                easing.type: Easing.OutQuad
            }
        }

        Rectangle {
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 4
            height: parent.height / 2 - 20
            color: root.needleColor
            radius: 2
        }

        // Pivot
        Rectangle {
            anchors.centerIn: parent
            width: 12
            height: 12
            radius: 6
            color: "#444444"
        }
    }

    // Label and Value
    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.bold: true
            font.pixelSize: 12
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.value.toFixed(1) + root.unit
            font.pixelSize: 16
            color: "#222222"
        }
    }
}
