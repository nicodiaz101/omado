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
        
        // Find custom list display name
        for (var i = 0; i < TaskListModel.rowCount(); ++i) {
            var idx = TaskListModel.index(i, 0)
            var id = TaskListModel.data(idx, 257) // IdRole
            if (id === TaskModel.currentListId) {
                return TaskListModel.data(idx, 258) // DisplayNameRole
            }
        }
        return qsTr("Tasks")
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

                        Text {
                            text: root.getListTitle()
                            color: Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

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
