import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0
import "."

Popup {
    id: popup
    width: 320
    height: Math.min(360, 68 + (listView.count > 0 ? Math.min(240, listView.count * 54) : 40))
    padding: 12
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Connections {
        target: TaskModel
        function onOverdueCountChanged() {
            if (TaskModel.overdueCount === 0) {
                popup.close()
            }
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        radius: 8
    }

    contentItem: ColumnLayout {
        id: contentCol
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppIcon {
                name: "sparkles"
                size: 14
                color: Theme.accent
            }

            Text {
                text: qsTr("Overdue Suggestions")
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 13
                font.bold: true
                Layout.fillWidth: true
            }

            Text {
                text: TaskModel.overdueCount + ""
                color: Theme.accent
                font.family: "iA Writer Mono"
                font.pixelSize: 11
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
            opacity: 0.5
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: count > 0 ? Math.min(240, count * 54) : 40
            Layout.preferredHeight: implicitHeight
            clip: true
            spacing: 6
            model: TaskModel.overdueSuggestions

            delegate: Rectangle {
                id: itemCard
                required property var modelData
                width: listView.width
                height: 48
                radius: 6
                color: itemArea.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05) : "transparent"
                border.color: itemArea.containsMouse ? Theme.border : "transparent"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: (modelData && modelData.title) ? modelData.title : ""
                            color: Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (modelData && modelData.dueDate && modelData.dueDate !== "") ? (qsTr("Due: ") + modelData.dueDate) : (qsTr("Overdue reminder"))
                            color: Theme.subtext
                            font.family: "iA Writer Mono"
                            font.pixelSize: 10
                        }
                    }

                    // Reschedule / Add to My Day button
                    Rectangle {
                        width: 26
                        height: 26
                        radius: 13
                        color: addArea.containsMouse ? Theme.accent : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)

                        AppIcon {
                            anchors.centerIn: parent
                            name: "sun"
                            size: 13
                            color: addArea.containsMouse ? Theme.background : Theme.accent
                        }

                        MouseArea {
                            id: addArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                TaskModel.rescheduleForMyDay(modelData.id)
                            }
                        }

                        ToolTip {
                            id: tip
                            visible: addArea.containsMouse
                            delay: 300
                            text: qsTr("Reschedule for My Day (Today)")
                            contentItem: Text {
                                text: tip.text
                                color: Theme.foreground
                                font.family: "iA Writer Mono"
                                font.pixelSize: 11
                            }
                            background: Rectangle {
                                color: Theme.surface
                                border.color: Theme.border
                                radius: 4
                            }
                        }
                    }
                }

                MouseArea {
                    id: itemArea
                    anchors.fill: parent
                    hoverEnabled: true
                    z: -1
                }
            }
        }
    }
}
