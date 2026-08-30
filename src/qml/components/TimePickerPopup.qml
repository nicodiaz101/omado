import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaDo.Theme 1.0

Popup {
    id: root
    width: 280
    height: 265
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
    property string customDateIso: ""

    function updateDefaults() {
        var now = new Date()
        root.currentHourNow = now.getHours()
        var nextHour = (now.getHours() + 1) % 24
        root.selectedHour = nextHour
        root.selectedMinute = 0
        root.selectedDayPreset = (now.getHours() >= 23) ? "tomorrow" : "today"
        root.customDateIso = ""
    }

    function saveAndClose() {
        hourInput.editingFinished()
        minuteInput.editingFinished()
        root.timeSelected(root.formatIso(root.selectedDayPreset, root.selectedHour, root.selectedMinute))
        root.close()
    }

    onAboutToShow: {
        updateDefaults()
        hourInput.text = (root.selectedHour < 10 ? "0" : "") + root.selectedHour
        minuteInput.text = (root.selectedMinute < 10 ? "0" : "") + root.selectedMinute
        hourInput.forceActiveFocus()
        hourInput.selectAll()
    }

    function formatIso(dayPreset, h, m) {
        var d = new Date()
        if (dayPreset === "tomorrow") {
            d.setDate(d.getDate() + 1)
        } else if (dayPreset === "custom" && root.customDateIso !== "") {
            var parts = root.customDateIso.split('-')
            if (parts.length === 3) {
                d = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]))
            }
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
        spacing: 8

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

        // Day Selector (Today / Tomorrow / Pick date...)
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Today button
            Rectangle {
                height: 26; Layout.fillWidth: true; radius: 4
                color: root.selectedDayPreset === "today" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: root.selectedDayPreset === "today" ? Theme.accent : "transparent"
                Text { anchors.centerIn: parent; text: qsTr("Today"); color: root.selectedDayPreset === "today" ? Theme.accent : Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.selectedDayPreset = "today"
                    }
                }
            }

            // Tomorrow button
            Rectangle {
                height: 26; Layout.fillWidth: true; radius: 4
                color: root.selectedDayPreset === "tomorrow" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: root.selectedDayPreset === "tomorrow" ? Theme.accent : "transparent"
                Text { anchors.centerIn: parent; text: qsTr("Tomorrow"); color: root.selectedDayPreset === "tomorrow" ? Theme.accent : Theme.foreground; font.family: "iA Writer Mono"; font.pixelSize: 11 }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.selectedDayPreset = "tomorrow"
                    }
                }
            }

            // Pick Date Calendar button
            Rectangle {
                id: pickDateBtn
                height: 26; Layout.fillWidth: true; radius: 4
                color: root.selectedDayPreset === "custom" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: root.selectedDayPreset === "custom" ? Theme.accent : "transparent"
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    AppIcon {
                        name: "calendar"
                        size: 11
                        color: root.selectedDayPreset === "custom" ? Theme.accent : Theme.foreground
                    }
                    Text {
                        text: (root.selectedDayPreset === "custom" && root.customDateIso !== "") ? root.customDateIso.substring(5) : qsTr("Pick date")
                        color: root.selectedDayPreset === "custom" ? Theme.accent : Theme.foreground
                        font.family: "iA Writer Mono"
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        embeddedDatePicker.open()
                    }
                }

                DatePickerPopup {
                    id: embeddedDatePicker
                    x: -width + parent.width
                    y: -height - 8
                    onDateSelected: function(iso) {
                        if (iso !== "") {
                            root.customDateIso = iso
                            root.selectedDayPreset = "custom"
                        }
                    }
                }
            }
        }

        // Custom Keyboard-Driven Time Inputs (00-23 : 00-59)
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            // Hour Input Box
            Rectangle {
                width: 58; height: 34
                radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: hourInput.activeFocus ? Theme.accent : Theme.border
                border.width: hourInput.activeFocus ? 1.5 : 1

                TextInput {
                    id: hourInput
                    anchors.centerIn: parent
                    text: (root.selectedHour < 10 ? "0" : "") + root.selectedHour
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 15
                    font.bold: true
                    maximumLength: 2
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: RegularExpressionValidator { regularExpression: /^([0-1]?[0-9]|2[0-3])$/ }

                    onTextEdited: {
                        var val = parseInt(text)
                        if (isNaN(val)) {
                            root.selectedHour = 0
                        } else if (val > 23) {
                            text = "23"
                            root.selectedHour = 23
                        } else if (val < 0) {
                            text = "00"
                            root.selectedHour = 0
                        } else {
                            root.selectedHour = val
                        }
                    }

                    onEditingFinished: {
                        var val = parseInt(text)
                        if (isNaN(val) || val < 0) val = 0
                        if (val > 23) val = 23
                        root.selectedHour = val
                        text = (val < 10 ? "0" : "") + val
                    }

                    Keys.onUpPressed: {
                        root.selectedHour = (root.selectedHour + 1) % 24
                        text = (root.selectedHour < 10 ? "0" : "") + root.selectedHour
                        selectAll()
                    }

                    Keys.onDownPressed: {
                        root.selectedHour = (root.selectedHour + 23) % 24
                        text = (root.selectedHour < 10 ? "0" : "") + root.selectedHour
                        selectAll()
                    }

                    Keys.onReturnPressed: root.saveAndClose()
                    Keys.onEnterPressed: root.saveAndClose()
                }
            }

            Text {
                text: ":"
                color: Theme.foreground
                font.family: "iA Writer Mono"
                font.bold: true
                font.pixelSize: 16
            }

            // Minute Input Box (Any minute 00-59)
            Rectangle {
                width: 58; height: 34
                radius: 4
                color: Qt.rgba(Theme.foreground.r, Theme.foreground.g, Theme.foreground.b, 0.05)
                border.color: minuteInput.activeFocus ? Theme.accent : Theme.border
                border.width: minuteInput.activeFocus ? 1.5 : 1

                TextInput {
                    id: minuteInput
                    anchors.centerIn: parent
                    text: (root.selectedMinute < 10 ? "0" : "") + root.selectedMinute
                    color: Theme.foreground
                    font.family: "iA Writer Mono"
                    font.pixelSize: 15
                    font.bold: true
                    maximumLength: 2
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: RegularExpressionValidator { regularExpression: /^[0-5]?[0-9]$/ }

                    onTextEdited: {
                        var val = parseInt(text)
                        if (isNaN(val)) {
                            root.selectedMinute = 0
                        } else if (val > 59) {
                            text = "59"
                            root.selectedMinute = 59
                        } else if (val < 0) {
                            text = "00"
                            root.selectedMinute = 0
                        } else {
                            root.selectedMinute = val
                        }
                    }

                    onEditingFinished: {
                        var val = parseInt(text)
                        if (isNaN(val) || val < 0) val = 0
                        if (val > 59) val = 59
                        root.selectedMinute = val
                        text = (val < 10 ? "0" : "") + val
                    }

                    Keys.onUpPressed: {
                        root.selectedMinute = (root.selectedMinute + 1) % 60
                        text = (root.selectedMinute < 10 ? "0" : "") + root.selectedMinute
                        selectAll()
                    }

                    Keys.onDownPressed: {
                        root.selectedMinute = (root.selectedMinute + 59) % 60
                        text = (root.selectedMinute < 10 ? "0" : "") + root.selectedMinute
                        selectAll()
                    }

                    Keys.onReturnPressed: root.saveAndClose()
                    Keys.onEnterPressed: root.saveAndClose()
                }
            }
        }

        // Action Buttons: Remove & Save
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
                    onClicked: root.saveAndClose()
                }
            }
        }
    }
}
