import QtQuick
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : parent.width
    height: 40
    color: TaskModel.selectedIndex === index 
           ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08)
           : (mouseArea.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.04) : "transparent")
    radius: 4

    property bool isCompleted: model.isCompleted
    property bool isHighImportance: model.importance === "high"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        // CheckCircle button
        Item {
            width: 24
            height: 24

            CheckCircle {
                anchors.centerIn: parent
                checked: root.isCompleted
                size: 18
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: TaskModel.toggleTaskCompletion(index)
            }
        }

        // Title and metadata column
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: model.title
                color: root.isCompleted ? Theme.border : Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 13
                font.strikeout: root.isCompleted
                elide: Text.ElideRight
            }

            // Subtitle badges (steps / due date / reminder)
            Row {
                spacing: 8
                visible: model.hasSteps || (model.dueDate !== "") || (model.reminderAt !== "")

                // Steps count badge
                Row {
                    spacing: 4
                    visible: model.hasSteps

                    AppIcon {
                        name: "steps"
                        size: 11
                        color: Theme.foreground
                        opacity: 0.5
                    }

                    Text {
                        text: model.completedStepsCount + "/" + model.stepsCount
                        color: Theme.foreground
                        opacity: 0.5
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }
                }

                // Due date badge
                Row {
                    spacing: 4
                    visible: model.dueDate !== ""

                    AppIcon {
                        name: "calendar"
                        size: 11
                        color: Theme.accent
                        opacity: 0.8
                    }

                    Text {
                        text: model.dueDate
                        color: Theme.accent
                        opacity: 0.8
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }
                }

                // Reminder badge
                Row {
                    spacing: 4
                    visible: model.reminderAt !== ""

                    AppIcon {
                        name: "alarm"
                        size: 11
                        color: Theme.accent
                        opacity: 0.8
                    }

                    Text {
                        text: model.reminderAt
                        color: Theme.accent
                        opacity: 0.8
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }
                }
            }
        }

        // Star / Importance Button
        Item {
            width: 24
            height: 24

            AppIcon {
                anchors.centerIn: parent
                name: "star"
                size: 16
                color: root.isHighImportance ? Theme.accent : Theme.border
                opacity: root.isHighImportance ? 1.0 : (mouseAreaStar.containsMouse ? 0.8 : 0.3)
            }

            MouseArea {
                id: mouseAreaStar
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: TaskModel.toggleTaskImportance(index)
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        z: -1
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            TaskModel.selectedIndex = index
        }
    }
}
