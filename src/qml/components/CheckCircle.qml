import QtQuick
import OmaDo.Theme 1.0

Rectangle {
    id: root
    property bool checked: false
    property int size: 18
    
    width: size
    height: size
    radius: size / 2
    
    color: checked ? Theme.accent : "transparent"
    border.color: checked ? Theme.accent : Theme.border
    border.width: 1.5
    
    Behavior on color { ColorAnimation { duration: 120 } }
    
    Text {
        anchors.centerIn: parent
        text: "✓"
        color: Theme.background
        font.pixelSize: root.size * 0.65
        font.bold: true
        visible: root.checked
    }
}
