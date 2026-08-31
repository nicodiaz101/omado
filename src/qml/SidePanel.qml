import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import OmaDo.Auth 1.0
import OmaDo.Sync 1.0
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

                    MouseArea {
                        id: mouseAreaList
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: TaskModel.currentListId = model.id
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
                            z: 2
                            width: 24
                            height: 24
                            visible: mouseAreaList.containsMouse || mouseAreaTrash.containsMouse

                            AppIcon {
                                anchors.centerIn: parent
                                name: "trash"
                                size: 13
                                color: mouseAreaTrash.containsMouse ? Theme.accent : Theme.foreground
                                opacity: mouseAreaTrash.containsMouse ? 1.0 : 0.6
                            }

                            MouseArea {
                                id: mouseAreaTrash
                                anchors.fill: parent
                                hoverEnabled: true
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

        // Footer: Login / Sync / User Info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: mouseAreaFooter.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.04) : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                // Indicator dot (Green when connected, Accent when authenticating)
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: AuthManager.isAuthenticated ? "#4CAF50" : (AuthManager.isAuthenticating ? Theme.accent : "transparent")
                    visible: AuthManager.isAuthenticated || AuthManager.isAuthenticating
                }

                Text {
                    Layout.fillWidth: true
                    text: {
                        if (AuthManager.isAuthenticating) {
                            return qsTr("Connecting...")
                        }
                        if (AuthManager.isAuthenticated) {
                            if (SyncEngine.isSyncing) return qsTr("Syncing...")
                            return AuthManager.userEmail !== "" ? AuthManager.userEmail : (AuthManager.userName !== "" ? AuthManager.userName : qsTr("Connected"))
                        }
                        return qsTr("Log in on MS To Do")
                    }
                    color: AuthManager.isAuthenticated ? Theme.foreground : Theme.accent
                    opacity: AuthManager.isAuthenticated ? 0.8 : 1.0
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                    font.bold: !AuthManager.isAuthenticated
                    elide: Text.ElideRight
                }

                // Sync button when authenticated
                Item {
                    width: 20
                    height: 20
                    visible: AuthManager.isAuthenticated

                    AppIcon {
                        anchors.centerIn: parent
                        name: "repeat"
                        size: 13
                        color: SyncEngine.isSyncing ? Theme.accent : Theme.foreground
                        opacity: SyncEngine.isSyncing ? 1.0 : (mouseAreaSync.containsMouse ? 0.9 : 0.5)
                    }

                    MouseArea {
                        id: mouseAreaSync
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            SyncEngine.syncNow()
                        }
                    }
                }
            }

            MouseArea {
                id: mouseAreaFooter
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (AuthManager.isAuthenticated) {
                        userMenu.open()
                    } else if (!AuthManager.isAuthenticating) {
                        AuthManager.startLogin()
                    }
                }
            }

            Menu {
                id: userMenu
                y: -height - 4

                MenuItem {
                    text: qsTr("Sync now (Ctrl+R)")
                    onTriggered: SyncEngine.syncNow()
                }
                MenuItem {
                    text: qsTr("Log out")
                    onTriggered: AuthManager.logout()
                }
            }
        }
    }

    Component.onCompleted: {
        TaskListModel.loadLists()
        TaskModel.currentListId = "special-myday"
    }
}
