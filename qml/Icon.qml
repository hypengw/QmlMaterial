import QtQuick
import QtQuick.Window
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: size
    implicitHeight: size

    property string name
    property int size: 24
    property int weight: Font.Normal
    property real fill: 0
    property alias horizontalAlignment: m_text_icon.horizontalAlignment
    property alias verticalAlignment: m_text_icon.verticalAlignment

    property int lineHeight: MD.Token.typescale.label_large.line_height
    property int iconStyle: MD.Enum.IconRound
    property color color: MD.MatProp.color.on_background

    Text {
        id: m_text_icon
        anchors.centerIn: parent
        renderType: Text.CurveRendering

        font.family: {
            switch (root.iconStyle) {
            case MD.Enum.IconRound:
            default:
                return MD.Token.font.icon_round_family;
            }
        }
        font.weight: root.weight
        // font.variableAxes: {
        //     "FILL": root.fill,
        //     "GRAD": 0,
        //     "opsz": root.size,
        //     "wght": root.weight
        // }
        font.pixelSize: root.size / scale
        scale: 1.0 / MD.Token.cal_curve_scale(Screen.devicePixelRatio)

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: root.name
        color: root.color
        lineHeight: root.lineHeight
        lineHeightMode: Text.FixedHeight
    }
}
