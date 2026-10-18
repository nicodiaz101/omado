import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0
import OmaDo.Models 1.0

Popup {
    id: root

    parent: Overlay.overlay

    property string targetListId: ""
    property string targetListName: ""
    property string targetListIcon: ""

    signal renameRequested(string listId, string currentName)
    signal changeIconRequested(string listId, string currentName, string currentIcon)

    width: 190
    padding: 6
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    function openAt(listId, name, icon, globalPos) {
        root.targetListId = listId
        root.targetListName = name
        root.targetListIcon = icon
        var pWidth = parent ? parent.width : 960
        var pHeight = parent ? parent.height : 620
        root.x = Math.max(8, Math.min(globalPos.x, pWidth - root.width - 8))
        root.y = Math.max(8, Math.min(globalPos.y, pHeight - root.height - 8))
        root.open()
    }

    ColumnLayout {
        width: parent.width
        spacing: 2

        // Change Icon option
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 6
            color: mouseAreaIcon.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Text {
                    text: root.targetListIcon !== "" ? root.targetListIcon : "☺"
                    font.pixelSize: 14
                    color: mouseAreaIcon.containsMouse ? Theme.accent : Theme.foreground
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Change icon")
                    color: mouseAreaIcon.containsMouse ? Theme.accent : Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: mouseAreaIcon
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var lid = root.targetListId
                    var lname = root.targetListName
                    var licon = root.targetListIcon
                    root.close()
                    root.changeIconRequested(lid, lname, licon)
                }
            }
        }

        // Rename List option
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 6
            color: mouseAreaRename.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Text {
                    text: "✎"
                    font.pixelSize: 14
                    color: Theme.foreground
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Rename list")
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: mouseAreaRename
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var lid = root.targetListId
                    var lname = root.targetListName
                    root.close()
                    root.renameRequested(lid, lname)
                }
            }
        }

        SectionSeparator {
            Layout.fillWidth: true
        }

        // Delete List option
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 6
            color: mouseAreaDelete.containsMouse ? Qt.rgba(1.0, 0.33, 0.33, 0.12) : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                AppIcon {
                    name: "trash"
                    size: 13
                    color: "#ff5555"
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Delete list")
                    color: "#ff5555"
                    font.family: "iA Writer Mono"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: mouseAreaDelete
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var lid = root.targetListId
                    root.close()
                    TaskListModel.deleteList(lid)
                }
            }
        }
    }
}
