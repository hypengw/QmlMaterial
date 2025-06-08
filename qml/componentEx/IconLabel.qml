import QtQuick
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: m_row.implicitWidth
    implicitHeight: m_row.implicitHeight

    property alias spacing: m_row.spacing

    property alias text: m_text.text
    property alias font: m_text.font
    property alias typescale: m_text.typescale
    property alias color: m_text.color

    property alias icon_name: m_icon.name
    property alias icon_size: m_icon.size
    property alias icon_color: m_icon.color

    property int icon_style: MD.Enum.IconAndText

    property int horizontalAlignment: Qt.AlignHCenter

    Row {
        id: m_row
        Binding {
            when: root.horizontalAlignment == Qt.AlignLeft
            m_row.anchors.left: root.left
        }
        Binding {
            when: root.horizontalAlignment == Qt.AlignRight
            m_row.anchors.right: root.right
        }
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: root.horizontalAlignment == Qt.AlignHCenter ? parent.horizontalCenter : undefined
        spacing: 8

        MD.Icon {
            id: m_icon
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon_style != MD.Enum.TextOnly && name.length > 0
            color: root.color
        }

        MD.Label {
            id: m_text
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: Text.AlignVCenter

            visible: root.icon_style != MD.Enum.IconOnly
            color: root.color
        }
    }
}
