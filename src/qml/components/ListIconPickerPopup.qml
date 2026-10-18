import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0

Popup {
    id: root

    parent: Overlay.overlay
    x: Math.max(16, Math.round((parent.width - width) / 2))
    y: Math.max(16, Math.round((parent.height - height) / 2))

    property string targetListId: ""
    property string targetListName: ""
    property string currentIcon: ""
    property int selectedCategory: 0

    width: 340
    height: 430
    padding: 14
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.4)
    }

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    function openForList(listId, name, icon) {
        root.targetListId = listId
        root.targetListName = name
        root.currentIcon = icon
        root.selectedCategory = 0
        customEmojiInput.text = ""
        root.open()
    }

    readonly property var categories: [
        {
            name: qsTr("General"),
            icon: "📋",
            emojis: [
                "📋", "📝", "📌", "🎯", "⭐", "💡",
                "🔔", "🔥", "⚡", "🚀", "🛠️", "🏷️",
                "🔑", "📦", "💬", "❗", "🏆", "💎",
                "🚩", "🕒", "📅", "✉️", "📎", "🔒"
            ]
        },
        {
            name: qsTr("Work"),
            icon: "💼",
            emojis: [
                "💼", "💻", "📚", "🎓", "📊", "📈",
                "💰", "💳", "🏢", "👔", "📞", "🖥️",
                "✏️", "📖", "🗂️", "📁", "🔬", "📐",
                "🧮", "🖊️", "🗃️", "🧾", "🗞️", "🖨️"
            ]
        },
        {
            name: qsTr("Home"),
            icon: "🏠",
            emojis: [
                "🏠", "🛒", "🧹", "🧺", "🍳", "🍎",
                "🍕", "🍔", "☕", "💊", "🏥", "🪴",
                "🌱", "🐶", "🐱", "🍼", "🛋️", "🚿",
                "🛌", "🥑", "🍓", "🍩", "🧁", "🍪"
            ]
        },
        {
            name: qsTr("Leisure"),
            icon: "✈️",
            emojis: [
                "✈️", "🚗", "🚲", "🏖️", "🏕️", "🎨",
                "🎵", "🎮", "⚽", "🏋️", "🎬", "📷",
                "🎁", "🎉", "🎈", "🎸", "🎟️", "🏄",
                "🧗", "🎿", "🎳", "🎧", "🎪", "🎲"
            ]
        },
        {
            name: qsTr("Symbols"),
            icon: "❤️",
            emojis: [
                "❤️", "🌟", "✨", "☀️", "🌙", "🌈",
                "💎", "🍀", "🌍", "🍻", "🧘", "🌸",
                "🌺", "🌻", "🍁", "🌲", "🌞", "⛅",
                "🌊", "🍕", "🎂", "🥂", "🥇", "💯"
            ]
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Header: Title and Close button
        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: qsTr("List icon")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 14
                font.bold: true
            }

            Rectangle {
                width: 24
                height: 24
                radius: 4
                color: mouseAreaClose.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.1) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: Theme.foreground
                    opacity: 0.7
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: mouseAreaClose
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        // Current icon preview & Reset button
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 6
            color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.04)
            border.color: Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.5)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Text {
                    text: root.currentIcon !== "" ? root.currentIcon : "≡"
                    font.pixelSize: 18
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    Layout.fillWidth: true
                    text: root.targetListName !== "" ? root.targetListName : qsTr("List")
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                // Reset to default button
                Rectangle {
                    Layout.preferredHeight: 26
                    Layout.preferredWidth: 95
                    radius: 4
                    color: mouseAreaReset.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : "transparent"
                    border.color: Theme.border
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Default")
                        color: Theme.foreground
                        opacity: 0.8
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }

                    MouseArea {
                        id: mouseAreaReset
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            TaskListModel.setListIcon(root.targetListId, "")
                            root.close()
                        }
                    }
                }
            }
        }

        // Category Tabs
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: root.categories

                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    radius: 5
                    color: root.selectedCategory === index ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16) : (mouseAreaCat.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.06) : "transparent")
                    border.color: root.selectedCategory === index ? Theme.accent : "transparent"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: modelData.icon
                            font.pixelSize: 12
                        }

                        Text {
                            text: modelData.name
                            color: root.selectedCategory === index ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                            font.bold: root.selectedCategory === index
                        }
                    }

                    MouseArea {
                        id: mouseAreaCat
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.selectedCategory = index
                    }
                }
            }
        }

        // Grid of Emojis
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: "transparent"
            border.color: Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.4)
            border.width: 1
            clip: true

            GridView {
                id: emojiGrid
                anchors.fill: parent
                anchors.margins: 6
                cellWidth: width / 6
                cellHeight: 44
                model: root.categories[root.selectedCategory].emojis
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: emojiGrid.cellWidth - 4
                    height: emojiGrid.cellHeight - 4
                    x: 2
                    y: 2
                    radius: 6
                    color: modelData === root.currentIcon
                           ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2)
                           : (mouseAreaEmoji.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent")
                    border.color: modelData === root.currentIcon ? Theme.accent : "transparent"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: mouseAreaEmoji
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            TaskListModel.setListIcon(root.targetListId, modelData)
                            root.close()
                        }
                    }
                }
            }
        }

        // Custom emoji input
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: 6
                color: Theme.surface
                border.color: customEmojiInput.activeFocus ? Theme.accent : Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8

                    TextInput {
                        id: customEmojiInput
                        Layout.fillWidth: true
                        color: Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 12
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true

                        Text {
                            text: qsTr("Or paste any emoji...")
                            color: Theme.foreground
                            opacity: 0.4
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                            visible: customEmojiInput.text.length === 0
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        onAccepted: {
                            if (text.trim().length > 0) {
                                TaskListModel.setListIcon(root.targetListId, text.trim())
                                root.close()
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 32
                radius: 6
                color: mouseAreaApply.containsMouse ? Theme.accent : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                border.color: Theme.accent
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Apply")
                    color: mouseAreaApply.containsMouse ? Theme.surface : Theme.accent
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea {
                    id: mouseAreaApply
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (customEmojiInput.text.trim().length > 0) {
                            TaskListModel.setListIcon(root.targetListId, customEmojiInput.text.trim())
                            root.close()
                        }
                    }
                }
            }
        }
    }
}
