import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0

Popup {
    id: root
    width: 280
    height: Math.min(260, 80 + draftList.count * 32)
    padding: 12
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var draftSteps: []
    signal stepsChanged(var newSteps)

    function addStep(title) {
        if (title.trim().length === 0) return
        var arr = draftSteps.slice()
        arr.push(title.trim())
        draftSteps = arr
        stepsChanged(draftSteps)
    }

    function removeStep(idx) {
        var arr = draftSteps.slice()
        arr.splice(idx, 1)
        draftSteps = arr
        stepsChanged(draftSteps)
    }

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Subtasks") + " (" + root.draftSteps.length + ")"
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 12
                font.bold: true
            }
        }

        // List of added draft steps
        ListView {
            id: draftList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.draftSteps

            delegate: Rectangle {
                width: draftList.width
                height: 28
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    CheckCircle {
                        checked: false
                        size: 14
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    AppIcon {
                        name: "close"
                        size: 12
                        color: Theme.foreground
                        opacity: 0.4

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.removeStep(index)
                        }
                    }
                }
            }
        }

        // Input to add another draft step
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppIcon {
                name: "plus"
                size: 13
                color: Theme.accent
            }

            TextInput {
                id: stepInput
                Layout.fillWidth: true
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 12
                selectByMouse: true

                Text {
                    text: qsTr("Add step...")
                    color: Theme.foreground
                    opacity: 0.35
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    visible: stepInput.text.length === 0
                    anchors.verticalCenter: parent.verticalCenter
                }

                onAccepted: {
                    if (text.trim().length > 0) {
                        root.addStep(text.trim())
                        text = ""
                    }
                }
            }
        }
    }
}
