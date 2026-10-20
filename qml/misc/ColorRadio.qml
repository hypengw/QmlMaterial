import QtQuick

import Qcm.Material as MD

MD.Selectable {
    id: root
    property color color
    property int size: 24

    implicitHeight: size
    implicitWidth: size

    Accessible.role: Accessible.RadioButton
    Accessible.checked: selected
    Accessible.checkable: true
    Accessible.onPressAction: click()

    Rectangle {
        anchors.centerIn: parent
        color: root.color
        height: width
        radius: width / 2
        width: parent.implicitWidth

        Rectangle {
            anchors.centerIn: parent
            border.color: root.MD.MProp.color.surface
            border.width: Math.max(1.2, width * 0.2)
            color: parent.color
            width: parent.width
            height: width
            radius: width / 2
            scale: root.selected ? 0.8 : 0.0
            visible: root.selected
        }
    }
}
