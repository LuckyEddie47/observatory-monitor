import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    property string status: "Unknown"

    width: 180
    height: 60
    color: "white"
    radius: 5
    border.color: "#cccccc"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        // Visual indicator
        Rectangle {
            width: 40
            height: 40
            radius: 5
            color: {
                if (root.status === "Open")
                    return "#4CAF50";    // Green
                if (root.status === "Closed")
                    return "#F44336";  // Red
                if (root.status === "Opening" || root.status === "Closing")
                    return "#FFC107"; // Amber
                return "#9E9E9E"; // Gray
            }

            // Simple shutter icon
            Rectangle {
                anchors.centerIn: parent
                width: 30
                height: 4
                color: "white"
                rotation: root.status === "Open" ? 90 : 0
                Behavior on rotation {
                    NumberAnimation {
                        duration: 1000
                    }
                }
            }
        }

        ColumnLayout {
            Label {
                text: "Shutter Status"
                font.bold: true
                font.pixelSize: 12
            }
            Label {
                text: root.status
                font.pixelSize: 16
                color: "#333333"
            }
        }
    }
}
