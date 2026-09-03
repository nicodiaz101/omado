import QtQuick
import QtQuick.Controls.impl
import OmaDo.Theme 1.0

Item {
    id: root
    property string name: ""
    property color color: Theme.foreground
    property int size: 16

    implicitWidth: size
    implicitHeight: size

    IconImage {
        anchors.fill: parent
        source: root.name ? "qrc:/OmaDo/icons/" + root.name + ".svg" : ""
        sourceSize.width: root.size * 2
        sourceSize.height: root.size * 2
        color: root.color
    }
}
