import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0

Popup {
    id: root
    width: 280
    height: 330
    padding: 12
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    signal dateSelected(string isoDate)

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    property date displayDate: new Date()
    property int currentMonth: displayDate.getMonth()
    property int currentYear: displayDate.getFullYear()

    readonly property var monthNames: [
        qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"),
        qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"),
        qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")
    ]

    function getDaysInMonth(year, month) {
        return new Date(year, month + 1, 0).getDate()
    }

    function getFirstDayOfWeek(year, month) {
        var day = new Date(year, month, 1).getDay()
        return (day === 0) ? 6 : day - 1 // Monday = 0
    }

    function formatDate(year, month, day) {
        var m = (month + 1).toString().padStart(2, '0')
        var d = day.toString().padStart(2, '0')
        return year + "-" + m + "-" + d
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // Month Navigation Header
        RowLayout {
            Layout.fillWidth: true

            Item {
                width: 24; height: 24
                Text { anchors.centerIn: parent; text: "‹"; color: Theme.foreground; font.pixelSize: 18 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var d = new Date(root.currentYear, root.currentMonth - 1, 1)
                        root.displayDate = d
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: root.monthNames[root.currentMonth] + " " + root.currentYear
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.pixelSize: 13
                font.bold: true
            }

            Item {
                width: 24; height: 24
                Text { anchors.centerIn: parent; text: "›"; color: Theme.foreground; font.pixelSize: 18 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var d = new Date(root.currentYear, root.currentMonth + 1, 1)
                        root.displayDate = d
                    }
                }
            }
        }

        // Days of week header
        Row {
            Layout.fillWidth: true
            spacing: 0
            Repeater {
                model: [qsTr("Mo"), qsTr("Tu"), qsTr("We"), qsTr("Th"), qsTr("Fr"), qsTr("Sa"), qsTr("Su")]
                Item {
                    width: (root.width - 24) / 7
                    height: 20
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: Theme.foreground
                        opacity: 0.4
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }
                }
            }
        }

        // Calendar Grid (42 cells = 6 weeks x 7 days)
        Grid {
            id: calendarGrid
            Layout.fillWidth: true
            columns: 7
            rows: 6
            spacing: 0

            property int daysInMonth: root.getDaysInMonth(root.currentYear, root.currentMonth)
            property int firstDay: root.getFirstDayOfWeek(root.currentYear, root.currentMonth)

            Repeater {
                model: 42
                delegate: Item {
                    width: (root.width - 24) / 7
                    height: 28

                    property int dayNumber: index - calendarGrid.firstDay + 1
                    property bool isValidDay: dayNumber >= 1 && dayNumber <= calendarGrid.daysInMonth
                    property bool isToday: {
                        var now = new Date()
                        return isValidDay && 
                               dayNumber === now.getDate() && 
                               root.currentMonth === now.getMonth() && 
                               root.currentYear === now.getFullYear()
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        radius: 12
                        color: isToday ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25) : 
                               (mouseAreaDay.containsMouse && isValidDay ? Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.08) : "transparent")
                        border.color: isToday ? Theme.accent : "transparent"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            visible: isValidDay
                            text: isValidDay ? dayNumber : ""
                            color: isToday ? Theme.accent : Theme.foreground
                            font.family: "iA Writer Mono"
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        id: mouseAreaDay
                        anchors.fill: parent
                        enabled: isValidDay
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var iso = root.formatDate(root.currentYear, root.currentMonth, dayNumber)
                            root.dateSelected(iso)
                            root.close()
                        }
                    }
                }
            }
        }

        // Quick Presets bar at the bottom
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 6

            Rectangle {
                Layout.fillWidth: true
                height: 24
                radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.06)
                Text { anchors.centerIn: parent; text: qsTr("Today"); color: Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var now = new Date()
                        root.dateSelected(root.formatDate(now.getFullYear(), now.getMonth(), now.getDate()))
                        root.close()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 24
                radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.06)
                Text { anchors.centerIn: parent; text: qsTr("Tomorrow"); color: Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var now = new Date()
                        now.setDate(now.getDate() + 1)
                        root.dateSelected(root.formatDate(now.getFullYear(), now.getMonth(), now.getDate()))
                        root.close()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 24
                radius: 4
                color: Qt.rgba(Theme.error.r, Theme.error.g, Theme.error.b, 0.08)
                Text { anchors.centerIn: parent; text: qsTr("Remove"); color: Theme.error; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.dateSelected("")
                        root.close()
                    }
                }
            }
        }
    }
}
