import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0

Popup {
    id: root
    width: 260
    height: 250
    padding: 12
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    signal timeSelected(string isoDateTime)

    background: Rectangle {
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    property int currentHourNow: new Date().getHours()
    property int selectedHour: 9
    property int selectedMinute: 0
    property string selectedDayPreset: "today"

    function updateDefaults() {
        var now = new Date()
        root.currentHourNow = now.getHours()
        var nextHour = (now.getHours() + 1) % 24
        root.selectedHour = nextHour
        root.selectedMinute = 0
        root.selectedDayPreset = (now.getHours() >= 23) ? "tomorrow" : "today"
    }

    onAboutToShow: {
        updateDefaults()
    }

    function formatIso(dayPreset, h, m) {
        var d = new Date()
        if (dayPreset === "tomorrow") {
            d.setDate(d.getDate() + 1)
        }
        var pad = function(n) { return n < 10 ? '0' + n : '' + n; }
        var year = d.getFullYear()
        var month = pad(d.getMonth() + 1)
        var day = pad(d.getDate())
        var hh = pad(h)
        var mm = pad(m)
        return year + "-" + month + "-" + day + "T" + hh + ":" + mm + ":00"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            text: qsTr("Set reminder")
            color: Theme.foreground
            font.family: "iA Writer Mono"
            font.pixelSize: 13
            font.bold: true
        }

        // Quick dynamic options
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            // Preset 1: Later today / Tonight / Tomorrow Afternoon
            Rectangle {
                Layout.fillWidth: true; height: 26; radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        if (root.currentHourNow < 18) return qsTr("Later today (18:00)")
                        if (root.currentHourNow < 21) return qsTr("Tonight (21:00)")
                        return qsTr("Tomorrow afternoon (14:00)")
                    }
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.currentHourNow < 18) {
                            root.timeSelected(root.formatIso("today", 18, 0))
                        } else if (root.currentHourNow < 21) {
                            root.timeSelected(root.formatIso("today", 21, 0))
                        } else {
                            root.timeSelected(root.formatIso("tomorrow", 14, 0))
                        }
                        root.close()
                    }
                }
            }

            // Preset 2: Tomorrow morning (09:00)
            Rectangle {
                Layout.fillWidth: true; height: 26; radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Tomorrow morning (09:00)")
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 11
                }

                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.timeSelected(root.formatIso("tomorrow", 9, 0))
                        root.close()
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; opacity: 0.5 }

        // Custom Time Selector
        Text {
            text: qsTr("Custom time:")
            color: Theme.foreground
            opacity: 0.5
            font.family: "iA Writer Mono"
            font.pixelSize: 11
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                height: 28; Layout.fillWidth: true; radius: 4
                color: root.selectedDayPreset === "today" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: root.selectedDayPreset === "today" ? Theme.accent : "transparent"
                Text { anchors.centerIn: parent; text: qsTr("Today"); color: root.selectedDayPreset === "today" ? Theme.accent : Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; onClicked: root.selectedDayPreset = "today" }
            }

            Rectangle {
                height: 28; Layout.fillWidth: true; radius: 4
                color: root.selectedDayPreset === "tomorrow" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: root.selectedDayPreset === "tomorrow" ? Theme.accent : "transparent"
                Text { anchors.centerIn: parent; text: qsTr("Tomorrow"); color: root.selectedDayPreset === "tomorrow" ? Theme.accent : Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; onClicked: root.selectedDayPreset = "tomorrow" }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            SpinBox {
                id: hourSpin
                Layout.fillWidth: true
                from: 0; to: 23; value: root.selectedHour
                onValueChanged: root.selectedHour = value
            }

            Text { text: ":"; color: Theme.foreground; font.bold: true; font.pixelSize: 16 }

            SpinBox {
                id: minuteSpin
                Layout.fillWidth: true
                from: 0; to: 55; stepSize: 5; value: root.selectedMinute
                onValueChanged: root.selectedMinute = value
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true; height: 26; radius: 4
                color: Qt.rgba(Theme.error.r, Theme.error.g, Theme.error.b, 0.08)
                Text { anchors.centerIn: parent; text: qsTr("Remove"); color: Theme.error; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: { root.timeSelected(""); root.close() }
                }
            }

            Rectangle {
                Layout.fillWidth: true; height: 26; radius: 4
                color: Theme.accent
                Text { anchors.centerIn: parent; text: qsTr("Save"); color: Theme.background; font.family: "iA Writer Mono"; font.pixelSize: 11; font.bold: true }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.timeSelected(root.formatIso(root.selectedDayPreset, root.selectedHour, root.selectedMinute))
                        root.close()
                    }
                }
            }
        }
    }
}
