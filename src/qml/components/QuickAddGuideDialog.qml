import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0

Dialog {
    id: dialog
    width: Math.min(560, parent ? parent.width - 40 : 560)
    height: Math.min(520, parent ? parent.height - 40 : 520)
    modal: true
    focus: true
    anchors.centerIn: parent

    property string selectedShortcut: "SUPER + ALT + T"
    property bool copiedFeedback: false
    property bool shortcutAddedFeedback: false

    function getShortcutDesc(key) {
        if (key === "SUPER + ALT + T") return qsTr("Tasks (Recommended)")
        if (key === "SUPER + ALT + D") return qsTr("OmaDo")
        if (key === "SUPER + ALT + N") return qsTr("New Task")
        if (key === "SUPER + SHIFT + ALT + T") return qsTr("Quick Action")
        return ""
    }

    onOpened: {
        var configured = TaskModel.configuredHyprlandShortcut()
        if (configured && configured !== "") {
            dialog.selectedShortcut = configured
        }
    }

    Timer {
        id: feedbackTimer
        interval: 2500
        repeat: false
        onTriggered: {
            dialog.copiedFeedback = false
            dialog.shortcutAddedFeedback = false
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        radius: 10
    }

    header: Rectangle {
        height: 50
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            AppIcon {
                name: "sparkles"
                size: 18
                color: Theme.accent
            }

            Text {
                text: qsTr("Quick Add & Hyprland Shortcut")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 15
                font.bold: true
                Layout.fillWidth: true
            }

            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: closeArea.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.1) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: Theme.subtext
                    font.pixelSize: 13
                }

                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dialog.close()
                }
            }
        }
    }

    contentItem: Flickable {
        clip: true
        contentWidth: width
        contentHeight: contentCol.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: contentCol
            width: parent.width
            spacing: 12

            // 1. Description
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Quick Add opens a floating minimalist input from anywhere in Omarchy. Type naturally and press Enter to save instantly to My Day without interrupting your work.")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 12
                opacity: 0.85
                lineHeight: 1.2
            }

            // 2. Natural language syntax examples
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 106
                radius: 8
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.04)
                border.color: Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.6)
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 5

                    Text {
                        text: qsTr("Natural Language Syntax:")
                        color: Theme.accent
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 8
                        Text { text: "•"; color: Theme.accent; font.pixelSize: 11 }
                        Text { text: qsTr("tomorrow at 6pm"); color: Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11; font.bold: true }
                        Text { text: "→ " + qsTr("Due date & Reminder"); color: Theme.subtext; font.family: "iA Writer Mono"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        spacing: 8
                        Text { text: "•"; color: Theme.accent; font.pixelSize: 11 }
                        Text { text: qsTr("#Work / #Shopping"); color: Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11; font.bold: true }
                        Text { text: "→ " + qsTr("Target list"); color: Theme.subtext; font.family: "iA Writer Mono"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        spacing: 8
                        Text { text: "•"; color: Theme.accent; font.pixelSize: 11 }
                        Text { text: "! / !!  " + qsTr("or") + "  " + qsTr("every week / daily"); color: Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11; font.bold: true }
                        Text { text: "→ " + qsTr("Priority & Recurrence"); color: Theme.subtext; font.family: "iA Writer Mono"; font.pixelSize: 10 }
                    }
                }
            }

            // 3. Shortcut Selection & Explanation
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: qsTr("Select Available Global Shortcut:")
                        color: Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    // Status Badge if already configured
                    Rectangle {
                        visible: TaskModel.isHyprlandShortcutConfigured() || dialog.shortcutAddedFeedback
                        height: 20
                        width: statusText.implicitWidth + 12
                        radius: 10
                        color: Qt.rgba(0.2, 0.8, 0.4, 0.2)
                        border.color: Qt.rgba(0.2, 0.8, 0.4, 0.8)

                        Text {
                            id: statusText
                            anchors.centerIn: parent
                            text: qsTr("Configured in bindings.lua")
                            color: "#2ee572"
                            font.family: "iA Writer Mono"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                // Chips for free shortcuts
                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { key: "SUPER + ALT + T", label: "SUPER + ALT + T" },
                            { key: "SUPER + ALT + D", label: "SUPER + ALT + D" },
                            { key: "SUPER + ALT + N", label: "SUPER + ALT + N" },
                            { key: "SUPER + SHIFT + ALT + T", label: "SUPER + SHIFT + ALT + T" }
                        ]

                        delegate: Rectangle {
                            id: chipRect
                            height: 28
                            width: chipRow.implicitWidth + 16
                            radius: 14
                            color: dialog.selectedShortcut === modelData.key
                                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                                   : (chipMouse.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.6))
                            border.color: dialog.selectedShortcut === modelData.key ? Theme.accent : Theme.border
                            border.width: dialog.selectedShortcut === modelData.key ? 1.5 : 1

                            RowLayout {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: 5

                                Text {
                                    text: modelData.label
                                    color: dialog.selectedShortcut === modelData.key ? Theme.accent : Theme.foreground
                                    font.family: "iA Writer Mono"
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Text {
                                    text: "• " + dialog.getShortcutDesc(modelData.key)
                                    color: dialog.selectedShortcut === modelData.key ? Theme.accent : Theme.subtext
                                    font.family: "iA Writer Mono"
                                    font.pixelSize: 9
                                    opacity: 0.85
                                }
                            }

                            MouseArea {
                                id: chipMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: dialog.selectedShortcut = modelData.key
                            }
                        }
                    }
                }

                // Global Shortcut Code Box
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 6
                    color: Qt.rgba(0, 0, 0, 0.35)
                    border.color: Theme.border
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8

                        Text {
                            Layout.fillWidth: true
                            text: 'o.bind("' + dialog.selectedShortcut + '", "OmaDo Quick Add", { launch = "omado quick add" })'
                            color: Theme.accent
                            font.family: "iA Writer Mono"
                            font.pixelSize: 10
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            width: copyBtnText.implicitWidth + 12
                            height: 22
                            radius: 4
                            color: copyArea.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25) : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
                            border.color: Theme.accent
                            border.width: 1

                            Text {
                                id: copyBtnText
                                anchors.centerIn: parent
                                text: dialog.copiedFeedback ? qsTr("Copied!") : qsTr("Copy")
                                color: Theme.accent
                                font.family: "iA Writer Mono"
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                id: copyArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    TaskModel.copyToClipboard('o.bind("' + dialog.selectedShortcut + '", "OmaDo Quick Add", { launch = "omado quick add" })')
                                    dialog.copiedFeedback = true
                                    feedbackTimer.restart()
                                }
                            }
                        }
                    }
                }
            }

            // 4. Action buttons at bottom
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 10

                // Button: Ask Omarchy Agent
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 6
                    color: agentArea.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3) : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                    border.color: Theme.accent
                    border.width: 1.5

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        AppIcon {
                            name: "sparkles"
                            size: 14
                            color: Theme.accent
                        }

                        Text {
                            text: qsTr("Ask Omarchy Agent")
                            color: Theme.accent
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: agentArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            TaskModel.launchOmarchyAgent(dialog.selectedShortcut)
                            dialog.close()
                        }
                    }
                }

                // Button: Add Shortcut (1-Click)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 6
                    color: directArea.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.12) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.06)
                    border.color: Theme.border
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: dialog.shortcutAddedFeedback ? qsTr("✓ Added & Reloaded!") : qsTr("Add Shortcut (1-Click)")
                            color: dialog.shortcutAddedFeedback ? "#2ee572" : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: directArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var ok = TaskModel.configureHyprlandShortcut(dialog.selectedShortcut)
                            if (ok) {
                                dialog.shortcutAddedFeedback = true
                                feedbackTimer.restart()
                            }
                        }
                    }
                }
            }
        }
    }
}
