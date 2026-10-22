import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

ApplicationWindow {
    id: quickAddWindow
    width: 600
    height: 120
    visible: true
    title: "OmaDo Quick Add"

    flags: Qt.FramelessWindowHint | Qt.Dialog
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.surface
        border.color: Theme.accent
        border.width: 1.5

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppIcon {
                    name: "sun"
                    size: 16
                    color: Theme.accent
                }

                Text {
                    text: qsTr("Quick Add Task")
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 13
                    font.bold: true
                    Layout.fillWidth: true
                }

                Text {
                    text: "Esc " + qsTr("to close")
                    color: Theme.subtext
                    font.family: "iA Writer Mono"
                    font.pixelSize: 10
                }
            }

            InputBar {
                id: inputBar
                Layout.fillWidth: true
                Layout.preferredHeight: 46

                Component.onCompleted: {
                    inputBar.focusInput()
                }
            }
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: quickAddWindow.close()
    }
}
