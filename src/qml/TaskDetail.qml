import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

Rectangle {
    id: root
    width: 320
    color: Theme.surface
    border.color: Theme.border
    border.width: 1
    radius: 6

    property var task: TaskModel.selectedTask
    property int selectedIndex: TaskModel.selectedIndex
    property bool isOpen: selectedIndex >= 0 && task && task.id !== undefined

    visible: opacity > 0
    opacity: isOpen ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 150 } }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 12

            // Top Header: Complete + Title + Star + Close
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(50, titleInput.implicitHeight + 20)
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    CheckCircle {
                        checked: (root.task && root.task.isCompleted !== undefined) ? root.task.isCompleted : false
                        size: 20

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: TaskModel.toggleTaskCompletion(root.selectedIndex)
                        }
                    }

                    TextInput {
                        id: titleInput
                        Layout.fillWidth: true
                        text: (root.task && root.task.title !== undefined) ? root.task.title : ""
                        color: Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 14
                        font.bold: true
                        wrapMode: TextInput.Wrap
                        selectByMouse: true

                        onEditingFinished: {
                            if (root.selectedIndex >= 0 && text.trim().length > 0) {
                                TaskModel.updateTaskTitle(root.selectedIndex, text.trim())
                            }
                        }
                    }

                    // Star / Importance
                    Item {
                        width: 24
                        height: 24

                        AppIcon {
                            anchors.centerIn: parent
                            name: "star"
                            size: 18
                            color: (root.task && root.task.importance === "high") ? Theme.accent : Theme.border
                            opacity: (root.task && root.task.importance === "high") ? 1.0 : 0.4
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: TaskModel.toggleTaskImportance(root.selectedIndex)
                        }
                    }

                    // Close Button
                    Item {
                        width: 24
                        height: 24

                        AppIcon {
                            anchors.centerIn: parent
                            name: "close"
                            size: 14
                            color: Theme.foreground
                            opacity: 0.6
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: TaskModel.selectedIndex = -1
                        }
                    }
                }
            }

            SectionSeparator { Layout.fillWidth: true }

            // Subtasks (Steps) Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                spacing: 6

                Text {
                    text: qsTr("Steps")
                    color: Theme.foreground
                    opacity: 0.5
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                // Existing steps list
                Repeater {
                    model: root.task ? root.task.steps : []

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            spacing: 8

                            CheckCircle {
                                checked: modelData.isCompleted
                                size: 14

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: TaskModel.toggleStep(root.selectedIndex, index)
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.title
                                color: modelData.isCompleted ? Theme.border : Theme.foreground
                                font.family: "iA Writer Mono"
                                font.pixelSize: 12
                                font.strikeout: modelData.isCompleted
                                elide: Text.ElideRight
                            }

                            AppIcon {
                                name: "close"
                                size: 12
                                color: Theme.foreground
                                opacity: 0.3

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: TaskModel.deleteStep(root.selectedIndex, index)
                                }
                            }
                        }
                    }
                }

                // Add step input
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    AppIcon {
                        name: "plus"
                        size: 13
                        color: Theme.accent
                    }

                    TextInput {
                        id: newStepInput
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
                            visible: newStepInput.text.length === 0
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        onAccepted: {
                            if (text.trim().length > 0 && root.selectedIndex >= 0) {
                                TaskModel.addStep(root.selectedIndex, text.trim())
                                text = ""
                            }
                        }
                    }
                }
            }

            SectionSeparator { Layout.fillWidth: true }

            // Metadata Options: My Day, Due Date, Reminder
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                spacing: 8

                // Toggle My Day
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    radius: 4
                    color: (root.task && root.task.isMyDay) ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.03)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        AppIcon {
                            name: "sun"
                            size: 15
                            color: (root.task && root.task.isMyDay) ? Theme.accent : Theme.foreground
                            opacity: (root.task && root.task.isMyDay) ? 1.0 : 0.6
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (root.task && root.task.isMyDay) ? qsTr("Added to My Day") : qsTr("Add to My Day")
                            color: (root.task && root.task.isMyDay) ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: TaskModel.toggleTaskMyDay(root.selectedIndex)
                    }
                }

                // Due Date Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    radius: 4
                    color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.03)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        AppIcon {
                            name: "calendar"
                            size: 15
                            color: (root.task && root.task.dueDate !== "") ? Theme.accent : Theme.foreground
                            opacity: 0.7
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (root.task && root.task.dueDate !== "") ? (qsTr("Due: ") + root.task.dueDate) : qsTr("Add due date")
                            color: (root.task && root.task.dueDate !== "") ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailDateMenu.open()
                    }

                    Menu {
                        id: detailDateMenu
                        MenuItem {
                            text: qsTr("Today")
                            onTriggered: TaskModel.updateTaskDueDate(root.selectedIndex, new Date().toISOString().split('T')[0])
                        }
                        MenuItem {
                            text: qsTr("Tomorrow")
                            onTriggered: {
                                var d = new Date()
                                d.setDate(d.getDate() + 1)
                                TaskModel.updateTaskDueDate(root.selectedIndex, d.toISOString().split('T')[0])
                            }
                        }
                        MenuItem {
                            text: qsTr("Remove due date")
                            onTriggered: TaskModel.updateTaskDueDate(root.selectedIndex, "")
                        }
                    }
                }

                // Reminder Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    radius: 4
                    color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.03)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        AppIcon {
                            name: "alarm"
                            size: 15
                            color: (root.task && root.task.reminderAt !== "") ? Theme.accent : Theme.foreground
                            opacity: 0.7
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (root.task && root.task.reminderAt !== "") ? (qsTr("Remind: ") + root.task.reminderAt) : qsTr("Remind me")
                            color: (root.task && root.task.reminderAt !== "") ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailReminderMenu.open()
                    }

                    Menu {
                        id: detailReminderMenu
                        MenuItem {
                            text: qsTr("Later today (18:00)")
                            onTriggered: {
                                var d = new Date()
                                d.setHours(18, 0, 0, 0)
                                TaskModel.updateTaskReminder(root.selectedIndex, d.toISOString())
                            }
                        }
                        MenuItem {
                            text: qsTr("Tomorrow (09:00)")
                            onTriggered: {
                                var d = new Date()
                                d.setDate(d.getDate() + 1)
                                d.setHours(9, 0, 0, 0)
                                TaskModel.updateTaskReminder(root.selectedIndex, d.toISOString())
                            }
                        }
                        MenuItem {
                            text: qsTr("Remove reminder")
                            onTriggered: TaskModel.updateTaskReminder(root.selectedIndex, "")
                        }
                    }
                }
            }

            SectionSeparator { Layout.fillWidth: true }

            // Notes / Body Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                spacing: 6

                Text {
                    text: qsTr("Notes")
                    color: Theme.foreground
                    opacity: 0.5
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    radius: 4
                    color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.03)
                    border.color: Theme.border
                    border.width: 1

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8

                        TextArea {
                            id: bodyArea
                            text: (root.task && root.task.body !== undefined) ? root.task.body : ""
                            color: Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 12
                            wrapMode: TextArea.Wrap
                            selectByMouse: true

                            Text {
                                text: qsTr("Add a note...")
                                color: Theme.foreground
                                opacity: 0.3
                                font.family: "iA Writer Mono"
                                font.pixelSize: 12
                                visible: bodyArea.text.length === 0
                            }

                            onEditingFinished: {
                                if (root.selectedIndex >= 0) {
                                    TaskModel.updateTaskBody(root.selectedIndex, text)
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.preferredHeight: 20 }

            // Bottom Bar: Delete Task Button
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Item {
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 30
                        radius: 4
                        color: Qt.rgba(Theme.error.r, Theme.error.g, Theme.error.b, 0.1)

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6

                            AppIcon {
                                name: "trash"
                                size: 14
                                color: Theme.error
                            }

                            Text {
                                text: qsTr("Delete")
                                color: Theme.error
                                font.family: "iA Writer Mono"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                TaskModel.deleteTask(root.selectedIndex)
                            }
                        }
                    }
                }
            }
        }
    }
}
