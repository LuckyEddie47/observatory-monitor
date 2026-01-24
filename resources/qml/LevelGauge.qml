import QtQuick

Item {
    id: root
    property double value: 0
    property double minValue: -90
    property double maxValue: 90
    property string unit: "°"
    property string label: ""
    property color barColor: "#2196F3"

    width: 60
    height: 200

    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        border.color: "#cccccc"
        radius: 3
    }

    // Scale markings
    Column {
        anchors.fill: parent
        anchors.margins: 5
        spacing: (parent.height - 10 - 10 * 1) / 10 // Approximate

        Repeater {
            model: 7
            delegate: Row {
                spacing: 5
                Rectangle {
                    width: index % 3 === 0 ? 15 : 8
                    height: 1
                    color: "#666666"
                }
                Text {
                    text: (root.maxValue - index * (root.maxValue - root.minValue) / 6).toFixed(0)
                    font.pixelSize: 8
                    color: "#666666"
                    visible: index % 3 === 0
                }
            }
        }
    }

    // Bar
    Rectangle {
        id: bar
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 20
        height: Math.max(0, Math.min(parent.height - 10, (root.value - root.minValue) / (root.maxValue - root.minValue) * (parent.height - 10)))
        color: root.barColor
        radius: 2

        Behavior on height {
            NumberAnimation {
                duration: 500
                easing.type: Easing.OutQuad
            }
        }
    }

    // Zero line
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: 1
        color: "red"
        visible: root.minValue < 0 && root.maxValue > 0
    }

    Text {
        anchors.top: parent.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        font.bold: true
        font.pixelSize: 10
    }

    Text {
        anchors.bottom: bar.top
        anchors.bottomMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.value.toFixed(1) + root.unit
        font.pixelSize: 10
        visible: bar.height > 20
    }
}
