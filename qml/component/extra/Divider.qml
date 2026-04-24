import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    property int size: 1
    property int orientation: Qt.Horizontal

    implicitWidth: orientation === Qt.Horizontal ? 0 : size
    implicitHeight: orientation === Qt.Horizontal ? size : 0

    color: MD.Token.color.outline_variant
}
