import QtQuick
import OmaDo.Theme 1.0

Item {
    width: parent.width
    height: 17
    
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: Theme.border
        opacity: 0.6
    }
}
