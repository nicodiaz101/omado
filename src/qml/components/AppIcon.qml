import QtQuick
import QtQuick.Effects
import OmaDo.Theme 1.0

Item {
    id: root
    property string name: ""
    property color color: Theme.foreground
    property int size: 16

    implicitWidth: size
    implicitHeight: size

    Image {
        id: img
        anchors.fill: parent
        source: root.name ? "qrc:/OmaDo/icons/" + root.name + ".svg" : ""
        sourceSize.width: root.size * 2
        sourceSize.height: root.size * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
    }

    MultiEffect {
        anchors.fill: img
        source: img
        colorization: 1.0
        colorizationColor: root.color
    }
}
