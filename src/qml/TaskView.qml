import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

Item {
    id: root

    property alias inputBar: inputBar

    function getListTitle() {
        if (TaskModel.currentListId === "special-myday") return qsTr("My Day")
        if (TaskModel.currentListId === "special-schedule") return qsTr("Schedule")
        if (TaskModel.currentListId === "special-tasks") return qsTr("Tasks")
        
        var name = TaskListModel.getListName(TaskModel.currentListId)
        if (name && name !== "") return name
        return qsTr("Tasks")
    }

    function getListIcon() {
        if (TaskModel.currentListId === "special-myday") return ""
        if (TaskModel.currentListId === "special-schedule") return ""
        if (TaskModel.currentListId === "special-tasks") return ""
        
        return TaskListModel.getListIcon(TaskModel.currentListId)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Main Tasks Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Top list title header (Stable alignment)
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 8

                        Text {
                            visible: root.getListIcon() !== ""
                            text: root.getListIcon()
                            font.pixelSize: 20
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: root.getListTitle()
                            color: Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 18
                            font.bold: true
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        // Overdue Suggestions Button (visible in My Day when overdueCount > 0)
                        Rectangle {
                            id: overdueBtn
                            visible: TaskModel.currentListId === "special-myday" && TaskModel.overdueCount > 0
                            height: 24
                            width: overdueRow.implicitWidth + 14
                            radius: 12
                            color: overdueArea.containsMouse
                                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                                   : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                            border.color: Theme.accent
                            border.width: 1

                            Row {
                                id: overdueRow
                                anchors.centerIn: parent
                                spacing: 5

                                AppIcon {
                                    name: "sparkles"
                                    size: 13
                                    color: Theme.accent
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    text: TaskModel.overdueCount + " " + qsTr("suggestions")
                                    color: Theme.accent
                                    font.family: "iA Writer Mono"
                                    font.pixelSize: 11
                                    font.bold: true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                id: overdueArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: suggestionsPopup.open()
                            }

                            MyDaySuggestionsPopup {
                                id: suggestionsPopup
                                y: parent.height + 6
                                x: parent.width - width
                            }
                        }

                        // Sort Button: Order by due date / reminder
                        Rectangle {
                            id: sortToggleBtn
                            height: 22
                            width: 22
                            radius: 11
                            color: TaskModel.sortByDueDate
                                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                                   : (mouseAreaSort.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.25))
                            border.color: TaskModel.sortByDueDate ? Theme.accent : (mouseAreaSort.containsMouse ? Theme.border : "transparent")
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 120 } }
                            Behavior on border.color { ColorAnimation { duration: 120 } }

                            AppIcon {
                                anchors.centerIn: parent
                                name: "arrow-down-up"
                                size: 13
                                color: TaskModel.sortByDueDate ? Theme.accent : Theme.foreground
                                opacity: TaskModel.sortByDueDate ? 1.0 : (mouseAreaSort.containsMouse ? 0.9 : 0.6)
                            }

                            MouseArea {
                                id: mouseAreaSort
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: TaskModel.sortByDueDate = !TaskModel.sortByDueDate
                            }

                            ToolTip {
                                id: sortToolTip
                                visible: mouseAreaSort.containsMouse
                                delay: 300
                                text: TaskModel.sortByDueDate ? qsTr("Restore default order") : qsTr("Sort by due date and reminder")

                                contentItem: Text {
                                    text: sortToolTip.text
                                    font.family: "iA Writer Mono"
                                    font.pixelSize: 11
                                    color: Theme.foreground
                                }
                                background: Rectangle {
                                    color: Theme.surface
                                    border.color: Theme.border
                                    border.width: 1
                                    radius: 4
                                }
                            }
                        }

                        // Filter Button: Hide / Show completed tasks
                        Rectangle {
                            id: filterToggleBtn
                            height: 22
                            width: 22
                            radius: 11
                            color: TaskModel.hideCompleted
                                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                                   : (mouseAreaFilter.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.25))
                            border.color: TaskModel.hideCompleted ? Theme.accent : (mouseAreaFilter.containsMouse ? Theme.border : "transparent")
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 120 } }
                            Behavior on border.color { ColorAnimation { duration: 120 } }

                            AppIcon {
                                anchors.centerIn: parent
                                name: TaskModel.hideCompleted ? "eye-off" : "eye"
                                size: 13
                                color: TaskModel.hideCompleted ? Theme.accent : Theme.foreground
                                opacity: TaskModel.hideCompleted ? 1.0 : (mouseAreaFilter.containsMouse ? 0.9 : 0.6)
                            }

                            MouseArea {
                                id: mouseAreaFilter
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: TaskModel.hideCompleted = !TaskModel.hideCompleted
                            }

                            ToolTip {
                                id: filterToolTip
                                visible: mouseAreaFilter.containsMouse
                                delay: 300
                                text: TaskModel.hideCompleted ? qsTr("Show completed tasks") : qsTr("Hide completed tasks")

                                contentItem: Text {
                                    text: filterToolTip.text
                                    font.family: "iA Writer Mono"
                                    font.pixelSize: 11
                                    color: Theme.foreground
                                }
                                background: Rectangle {
                                    color: Theme.surface
                                    border.color: Theme.border
                                    border.width: 1
                                    radius: 4
                                }
                            }
                        }

                        // Count Badge
                        Rectangle {
                            height: 22
                            width: countText.implicitWidth + 16
                            radius: 11
                            color: Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.4)

                            Text {
                                id: countText
                                anchors.centerIn: parent
                                text: TaskModel.count + (TaskModel.count === 1 ? " " + qsTr("task") : " " + qsTr("tasks"))
                                color: Theme.foreground
                                opacity: 0.7
                                font.family: "iA Writer Mono"
                                font.pixelSize: 11
                            }
                        }
                    }

                    DragHandler {
                        target: null
                        onActiveChanged: if (active) window.startSystemMove()
                    }
                }

                // Tasks List
                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.bottomMargin: 8
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    model: TaskModel
                    delegate: TaskDelegate {}

                    // Empty state placeholder
                    Column {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: TaskModel.count === 0

                        AppIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            name: "tasks"
                            size: 32
                            color: Theme.border
                            opacity: 0.6
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("No pending tasks")
                            color: Theme.foreground
                            opacity: 0.4
                            font.family: "iA Writer Mono"
                            font.pixelSize: 13
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Type below or press N to create one")
                            color: Theme.foreground
                            opacity: 0.25
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                        }
                    }
                }

                // InputBar fixed at the bottom
                InputBar {
                    id: inputBar
                    Layout.fillWidth: true
                }
            }
        }

        // Slide-in Task Detail Panel
        TaskDetail {
            id: detailPanel
            Layout.fillHeight: true
            Layout.preferredWidth: isOpen ? 320 : 0
            visible: isOpen
        }
    }
}
