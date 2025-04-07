import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: layout_row.implicitWidth
    implicitHeight: layout_row.implicitHeight

    property alias spacing: layout_row.spacing

    property alias text: item_label_text.text
    property alias font: item_label_text.font
    property alias typescale: item_label_text.typescale
    property alias color: item_label_text.color

    property alias icon_name: item_label_icon.name
    property alias icon_size: item_label_icon.size
    property alias icon_color: item_label_icon.color

    property int icon_style: MD.Enum.IconAndText

    property int horizontalAlignment: Qt.AlignHCenter

    RowLayout {
        id: layout_row

        anchors.fill: parent
        spacing: 8

        MD.Icon {
            id: item_label_icon
            Layout.alignment: root.horizontalAlignment | Qt.AlignVCenter
            visible: root.icon_style != MD.Enum.TextOnly && name.length > 0
            color: root.color
        }

        MD.Label {
            id: item_label_text
            Layout.alignment: root.horizontalAlignment | Qt.AlignVCenter
            verticalAlignment: Text.AlignVCenter

            visible: root.icon_style != MD.Enum.IconOnly
            color: root.color
        }

        Item {
            Layout.fillWidth: true
            visible: root.horizontalAlignment == Qt.AlignLeft
        }
    }
}
