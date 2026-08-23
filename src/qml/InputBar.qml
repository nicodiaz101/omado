import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

Rectangle {
    id: root
    height: 46
    color: Theme.surface

    property string draftDueDate: ""
    property string draftReminderAt: ""
    property string draftRecurrence: "none"
    property var draftSteps: []

    function focusInput() {
        taskInput.forceActiveFocus()
    }

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
        opacity: 0.7
        anchors.top: parent.top
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10

        CheckCircle {
            checked: false
            size: 18
        }

        TextInput {
            id: taskInput
            Layout.fillWidth: true
            color: Theme.foreground
            font.family: "iA Writer Mono"
            font.pixelSize: 13
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true

            Text {
                text: qsTr("Add a task...")
                color: Theme.foreground
                opacity: 0.4
                font.family: "iA Writer Mono"
                font.pixelSize: 13
                visible: taskInput.text.length === 0
                anchors.verticalCenter: parent.verticalCenter
            }

            onAccepted: {
                if (text.trim().length > 0) {
                    TaskModel.addTaskWithSteps(text.trim(), root.draftDueDate, root.draftReminderAt, root.draftRecurrence, root.draftSteps)
                    text = ""
                    root.draftDueDate = ""
                    root.draftReminderAt = ""
                    root.draftRecurrence = "none"
                    root.draftSteps = []
                    draftStepsPopup.draftSteps = []
                }
            }
        }

        // Active filters/draft badges preview
        Row {
            spacing: 6
            visible: root.draftDueDate !== "" || root.draftRecurrence !== "none" || root.draftSteps.length > 0

            // Due date badge
            Rectangle {
                visible: root.draftDueDate !== ""
                height: 20
                width: badgeDateText.implicitWidth + 12
                radius: 10
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)

                Text {
                    id: badgeDateText
                    anchors.centerIn: parent
                    text: root.draftDueDate
                    color: Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 10
                }
            }

            // Recurrence badge
            Rectangle {
                visible: root.draftRecurrence !== "none"
                height: 20
                width: badgeRecurrenceText.implicitWidth + 12
                radius: 10
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)

                Text {
                    id: badgeRecurrenceText
                    anchors.centerIn: parent
                    text: {
                        if (root.draftRecurrence === "daily") return qsTr("Daily")
                        if (root.draftRecurrence === "workdays") return qsTr("Weekdays")
                        if (root.draftRecurrence === "weekly") return qsTr("Weekly")
                        if (root.draftRecurrence === "monthly") return qsTr("Monthly")
                        return ""
                    }
                    color: Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 10
                }
            }

            // Subtasks count badge
            Rectangle {
                visible: root.draftSteps.length > 0
                height: 20
                width: badgeStepsText.implicitWidth + 12
                radius: 10
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)

                Text {
                    id: badgeStepsText
                    anchors.centerIn: parent
                    text: root.draftSteps.length + (root.draftSteps.length === 1 ? " " + qsTr("subtask") : " " + qsTr("subtasks"))
                    color: Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 10
                }
            }
        }

        // 4 Action Buttons with crisp SVG vector icons
        Row {
            spacing: 14

            // 1. Due Date (Calendar)
            Item {
                width: 22
                height: 22

                AppIcon {
                    anchors.centerIn: parent
                    name: "calendar"
                    size: 16
                    color: root.draftDueDate !== "" ? Theme.accent : Theme.foreground
                    opacity: root.draftDueDate !== "" ? 1.0 : (mouseDate.containsMouse ? 0.9 : 0.45)
                }

                MouseArea {
                    id: mouseDate
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        datePicker.open()
                    }
                }

                DatePickerPopup {
                    id: datePicker
                    x: -width + parent.width
                    y: -height - 8
                    onDateSelected: function(iso) {
                        root.draftDueDate = iso
                    }
                }
            }

            // 2. Reminder (Alarm)
            Item {
                width: 22
                height: 22

                AppIcon {
                    anchors.centerIn: parent
                    name: "alarm"
                    size: 16
                    color: root.draftReminderAt !== "" ? Theme.accent : Theme.foreground
                    opacity: root.draftReminderAt !== "" ? 1.0 : (mouseAlarm.containsMouse ? 0.9 : 0.45)
                }

                MouseArea {
                    id: mouseAlarm
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        timePicker.open()
                    }
                }

                TimePickerPopup {
                    id: timePicker
                    x: -width + parent.width
                    y: -height - 8
                    onTimeSelected: function(iso) {
                        root.draftReminderAt = iso
                    }
                }
            }

            // 3. Recurrence (Repeat)
            Item {
                width: 22
                height: 22

                AppIcon {
                    anchors.centerIn: parent
                    name: "repeat"
                    size: 16
                    color: root.draftRecurrence !== "none" ? Theme.accent : Theme.foreground
                    opacity: root.draftRecurrence !== "none" ? 1.0 : (mouseRepeat.containsMouse ? 0.9 : 0.45)
                }

                MouseArea {
                    id: mouseRepeat
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        recurrencePopup.open()
                    }
                }

                RecurrencePopup {
                    id: recurrencePopup
                    x: -width + parent.width
                    y: -height - 8
                    onRecurrenceSelected: function(rec) {
                        root.draftRecurrence = rec
                    }
                }
            }

            // 4. Subtasks (Steps)
            Item {
                width: 22
                height: 22

                AppIcon {
                    anchors.centerIn: parent
                    name: "steps"
                    size: 16
                    color: root.draftSteps.length > 0 ? Theme.accent : Theme.foreground
                    opacity: root.draftSteps.length > 0 ? 1.0 : (mouseSteps.containsMouse ? 0.9 : 0.45)
                }

                MouseArea {
                    id: mouseSteps
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        draftStepsPopup.open()
                    }
                }

                DraftStepsPopup {
                    id: draftStepsPopup
                    x: -width + parent.width
                    y: -height - 8
                    onStepsChanged: function(newSteps) {
                        root.draftSteps = newSteps
                    }
                }
            }
        }
    }
}
