import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

ApplicationWindow {
    id: quickAddWindow
    width: 600
    height: 104
    visible: true
    title: "OmaDo Quick Add"

    property string initialText: ""

    flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.WindowStaysOnTopHint
    color: "transparent"

    x: Screen.width > width ? Math.round((Screen.width - width) / 2) : 0
    y: Screen.height > height ? Math.round((Screen.height - height) / 3) : 0

    Component.onCompleted: {
        quickAddWindow.requestActivate()
    }

    Timer {
        interval: 50
        running: true
        repeat: false
        onTriggered: {
            quickAddWindow.requestActivate()
            inputBar.focusInput()
        }
    }

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

                RowLayout {
                    spacing: 6

                    Text {
                        text: "Esc " + qsTr("to close")
                        color: Theme.subtext
                        font.family: "iA Writer Mono"
                        font.pixelSize: 10
                    }

                    Item {
                        width: 16
                        height: 16

                        AppIcon {
                            anchors.centerIn: parent
                            name: "close"
                            size: 12
                            color: Theme.subtext
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: quickAddWindow.close()
                        }
                    }
                }
            }

            InputBar {
                id: inputBar
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                showActionButtons: false
                text: quickAddWindow.initialText

                onTaskAdded: {
                    quickAddWindow.close()
                }

                Component.onCompleted: {
                    if (!TaskModel.currentListId) {
                        TaskModel.currentListId = "special-myday"
                    }
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
