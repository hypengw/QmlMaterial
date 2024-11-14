import QtQuick
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: size
    implicitHeight: size

    property string name
    property int size: 24
    property alias horizontalAlignment: m_text_icon.horizontalAlignment
    property alias verticalAlignment: m_text_icon.verticalAlignment

    property int lineHeight: MD.Token.typescale.label_large.line_height
    property int iconStyle: MD.Enum.IconRound
    property color color: MD.MatProp.color.on_background

    Text {
        id: m_text_icon
        anchors.centerIn: parent

        font.family: {
            switch (root.iconStyle) {
            case MD.Enum.IconRound:
            default:
                return MD.Token.font.icon_round.family;
            }
        }

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: root.size
        text: root.name
        color: root.color
        lineHeight: root.lineHeight
        lineHeightMode: Text.FixedHeight
    }
}
