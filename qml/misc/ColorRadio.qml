import QtQuick

import Qcm.Material as MD

Item {
    id: root
    property color color
    property int size: 24
    property bool checked: false

    implicitHeight: size
    implicitWidth: size

    signal clicked

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
            scale: root.checked ? 0.8 : 0.0
            visible: root.checked
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true

            onClicked: root.clicked()
        }
    }
}
