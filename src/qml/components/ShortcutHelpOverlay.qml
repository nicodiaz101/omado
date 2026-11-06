import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0

Dialog {
    id: dialog
    width: 420
    height: 380
    modal: true
    focus: true
    anchors.centerIn: parent

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        radius: 8
    }

    header: Rectangle {
        height: 48
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16

            Text {
                text: qsTr("Keyboard Shortcuts")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 15
                font.bold: true
                Layout.fillWidth: true
            }

            Rectangle {
                width: 20
                height: 20
                radius: 10
                color: closeArea.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.1) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: Theme.subtext
                    font.pixelSize: 12
                }

                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dialog.close()
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        Repeater {
            model: [
                { key: "N", desc: qsTr("New task (focus input)") },
                { key: "j / Down", desc: qsTr("Next task") },
                { key: "k / Up", desc: qsTr("Previous task") },
                { key: "Space", desc: qsTr("Toggle complete") },
                { key: "Delete", desc: qsTr("Delete selected task") },
                { key: "Esc", desc: qsTr("Deselect / Close panel") },
                { key: "Ctrl + 1", desc: qsTr("Go to My Day") },
                { key: "Ctrl + 2", desc: qsTr("Go to Schedule") },
                { key: "Ctrl + 3", desc: qsTr("Go to Tasks") },
                { key: "Ctrl + R", desc: qsTr("Sync with Microsoft To Do") },
                { key: "?", desc: qsTr("Show this help dialog") }
            ]

            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 22
                    radius: 4
                    color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08)
                    border.color: Theme.border
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData.key
                        color: Theme.accent
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Text {
                    text: modelData.desc
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }
        }
    }
}
