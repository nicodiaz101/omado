import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import OmaDo.Sync 1.0
import OmaDo.Auth 1.0
import "components"

ApplicationWindow {
    id: window
    width: 960
    height: 620
    minimumWidth: 640
    minimumHeight: 460
    visible: true
    title: "OmaDo"

    color: Theme.background

    ShortcutHelpOverlay {
        id: helpOverlay
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background
        clip: true

        RowLayout {
            anchors.fill: parent
            spacing: 0

            SidePanel {
                Layout.fillHeight: true
                Layout.preferredWidth: 240
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: Theme.border
                opacity: 0.6
            }

            TaskView {
                id: taskView
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }

    // Keyboard Shortcuts for full productivity
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: {
            if (AuthManager.isAuthenticated) {
                SyncEngine.syncNow()
            }
        }
    }

    Shortcut {
        sequence: "N"
        onActivated: taskView.inputBar.focusInput()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (TaskModel.selectedIndex >= 0) {
                TaskModel.selectedIndex = -1
            }
        }
    }

    Shortcut {
        sequence: "Delete"
        onActivated: {
            if (TaskModel.selectedIndex >= 0) {
                TaskModel.deleteTask(TaskModel.selectedIndex)
            }
        }
    }

    Shortcut {
        sequence: "Space"
        onActivated: {
            if (TaskModel.selectedIndex >= 0) {
                TaskModel.toggleTaskCompletion(TaskModel.selectedIndex)
            }
        }
    }

    Shortcut {
        sequence: "Down"
        onActivated: {
            if (TaskModel.count > 0) {
                if (TaskModel.selectedIndex < TaskModel.count - 1) {
                    TaskModel.selectedIndex++
                } else if (TaskModel.selectedIndex < 0) {
                    TaskModel.selectedIndex = 0
                }
            }
        }
    }

    Shortcut {
        sequence: "J"
        onActivated: {
            if (TaskModel.count > 0) {
                if (TaskModel.selectedIndex < TaskModel.count - 1) {
                    TaskModel.selectedIndex++
                } else if (TaskModel.selectedIndex < 0) {
                    TaskModel.selectedIndex = 0
                }
            }
        }
    }

    Shortcut {
        sequence: "Up"
        onActivated: {
            if (TaskModel.count > 0) {
                if (TaskModel.selectedIndex > 0) {
                    TaskModel.selectedIndex--
                }
            }
        }
    }

    Shortcut {
        sequence: "K"
        onActivated: {
            if (TaskModel.count > 0) {
                if (TaskModel.selectedIndex > 0) {
                    TaskModel.selectedIndex--
                }
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+1"
        onActivated: TaskModel.currentListId = "special-myday"
    }

    Shortcut {
        sequence: "Ctrl+2"
        onActivated: TaskModel.currentListId = "special-schedule"
    }

    Shortcut {
        sequence: "Ctrl+3"
        onActivated: TaskModel.currentListId = "special-tasks"
    }

    Shortcut {
        sequence: "?"
        onActivated: helpOverlay.open()
    }
}
