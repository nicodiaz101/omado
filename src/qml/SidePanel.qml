import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "components"

Rectangle {
    id: root
    width: 240
    color: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Drag handle for moving window
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 16
            DragHandler {
                target: null
                onActiveChanged: if (active) window.startSystemMove()
            }
        }

        // Section 1: Special Lists (My Day, Schedule, Tasks)
        Column {
            Layout.fillWidth: true
            spacing: 2

            Repeater {
                model: [
                    { id: "special-myday", name: qsTr("My Day"), icon: "sun" },
                    { id: "special-schedule", name: qsTr("Schedule"), icon: "calendar" },
                    { id: "special-tasks", name: qsTr("Tasks"), icon: "tasks" }
                ]

                delegate: Rectangle {
                    width: root.width
                    height: 34
                    color: TaskModel.currentListId === modelData.id ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"

                    Rectangle {
                        width: 3
                        height: parent.height
                        color: Theme.accent
                        visible: TaskModel.currentListId === modelData.id
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        AppIcon {
                            name: modelData.icon
                            size: 16
                            color: TaskModel.currentListId === modelData.id ? Theme.accent : Theme.foreground
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: TaskModel.currentListId === modelData.id ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 13
                            font.bold: TaskModel.currentListId === modelData.id
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            TaskModel.currentListId = modelData.id
                        }
                    }
                }
            }
        }

        SectionSeparator {
            Layout.fillWidth: true
        }

        // Section 2: User Custom Lists
        ListView {
            id: userListsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: TaskListModel
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                // Filter out special lists so they only appear in Section 1
                visible: !model.isSpecial && model.id !== "default-tasks"
                width: userListsView.width
                height: visible ? 34 : 0

                Rectangle {
                    anchors.fill: parent
                    color: TaskModel.currentListId === model.id ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"

                    Rectangle {
                        width: 3
                        height: parent.height
                        color: Theme.accent
                        visible: TaskModel.currentListId === model.id
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12
                        spacing: 12

                        AppIcon {
                            name: "list"
                            size: 15
                            color: TaskModel.currentListId === model.id ? Theme.accent : Theme.foreground
                        }

                        Text {
                            Layout.fillWidth: true
                            text: model.displayName
                            color: TaskModel.currentListId === model.id ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }

                        // Delete list button (hover)
                        Item {
                            width: 20
                            height: 20
                            visible: mouseAreaList.containsMouse

                            AppIcon {
                                anchors.centerIn: parent
                                name: "trash"
                                size: 13
                                color: Theme.foreground
                                opacity: 0.6
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (TaskModel.currentListId === model.id) {
                                        TaskModel.currentListId = "special-myday"
                                    }
                                    TaskListModel.deleteList(model.id)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: mouseAreaList
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: TaskModel.currentListId = model.id
                    }
                }
            }
        }

        // Add new list input
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                AppIcon {
                    name: "plus"
                    size: 14
                    color: Theme.accent
                }

                TextInput {
                    id: newListInput
                    Layout.fillWidth: true
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 13
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true

                    Text {
                        text: qsTr("New list...")
                        color: Theme.foreground
                        opacity: 0.4
                        font.family: "iA Writer Mono"
                        font.pixelSize: 13
                        visible: newListInput.text.length === 0
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    onAccepted: {
                        if (text.trim().length > 0) {
                            TaskListModel.createList(text.trim())
                            text = ""
                        }
                    }
                }
            }
        }

        // SectionSeparator before Footer
        SectionSeparator {
            Layout.fillWidth: true
        }

        // Footer: Login to MS To Do
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Log in on MS To Do")
                    color: Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    // Auth flow trigger (Hito 3)
                }
            }
        }
    }

    Component.onCompleted: {
        TaskListModel.loadLists()
        TaskModel.currentListId = "special-myday"
    }
}
