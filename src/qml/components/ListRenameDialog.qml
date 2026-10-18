import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0

Popup {
    id: root

    parent: Overlay.overlay
    x: Math.max(16, Math.round((parent.width - width) / 2))
    y: Math.max(16, Math.round((parent.height - height) / 2))

    property string targetListId: ""
    property string targetListName: ""

    width: 290
    height: 150
    padding: 14
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.4)
    }

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    function openForList(listId, name) {
        root.targetListId = listId
        root.targetListName = name
        renameTextInput.text = name
        root.open()
        renameTextInput.forceActiveFocus()
        renameTextInput.selectAll()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: qsTr("Rename list")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 13
                font.bold: true
            }

            Rectangle {
                width: 22
                height: 22
                radius: 4
                color: mouseAreaClose.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.1) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: Theme.foreground
                    opacity: 0.7
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                MouseArea {
                    id: mouseAreaClose
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        // Input Box
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            radius: 6
            color: Theme.surface
            border.color: renameTextInput.activeFocus ? Theme.accent : Theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                TextInput {
                    id: renameTextInput
                    Layout.fillWidth: true
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true

                    onAccepted: {
                        if (text.trim().length > 0) {
                            TaskListModel.renameList(root.targetListId, text.trim())
                            root.close()
                        }
                    }
                }
            }
        }

        // Actions: Save / Cancel
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 28
                radius: 5
                color: mouseAreaCancel.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : "transparent"
                border.color: Theme.border
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Cancel")
                    color: Theme.foreground
                    opacity: 0.8
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                MouseArea {
                    id: mouseAreaCancel
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }

            Rectangle {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 28
                radius: 5
                color: mouseAreaSave.containsMouse ? Theme.accent : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                border.color: Theme.accent
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Save")
                    color: mouseAreaSave.containsMouse ? Theme.surface : Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea {
                    id: mouseAreaSave
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (renameTextInput.text.trim().length > 0) {
                            TaskListModel.renameList(root.targetListId, renameTextInput.text.trim())
                            root.close()
                        }
                    }
                }
            }
        }
    }
}
