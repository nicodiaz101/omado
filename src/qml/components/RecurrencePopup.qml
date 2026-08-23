import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0

Popup {
    id: root
    width: 220
    height: 180
    padding: 8
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    signal recurrenceSelected(string recurrence)

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: [
                { id: "none", label: qsTr("Never") },
                { id: "daily", label: qsTr("Daily") },
                { id: "workdays", label: qsTr("Weekdays (Mon-Fri)") },
                { id: "weekly", label: qsTr("Weekly") },
                { id: "monthly", label: qsTr("Monthly") }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: mouseAreaItem.containsMouse ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    Text {
                        Layout.fillWidth: true
                        text: modelData.label
                        color: Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    id: mouseAreaItem
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.recurrenceSelected(modelData.id)
                        root.close()
                    }
                }
            }
        }
    }
}
