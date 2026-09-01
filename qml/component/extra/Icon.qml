import QtQuick
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: size
    implicitHeight: size

    property string name
    property int size: 24
    property int weight: Font.Normal
    property bool fill: false
    property alias horizontalAlignment: m_text_icon.horizontalAlignment
    property alias verticalAlignment: m_text_icon.verticalAlignment

    property color color: MD.MProp.color.on_background

    property real _fill: fill ? 1 : 0
    Behavior on _fill {
        NumberAnimation {
            duration: 150
        }
    }

    readonly property var _fillVals: ([0, 0.25, 0.5, 0.75, 1])
    readonly property real _fillSeg: {
        let v = 0;
        if (fill) {
            v = _fillVals.find(el => _fill <= el);
        } else {
            v = MD.Util.findLast(_fillVals, el => _fill >= el);
        }
        return v;
    }

    readonly property real _parentScale: {
        var s = 1.0;
        for (var p = root.parent; p; p = p.parent)
            s *= p.scale;
        return s;
    }
    readonly property real _renderScale: {
        if (MD.Util.epsilonEqual(_parentScale, 1.0))
            return 1.0;
        return _parentScale;
    }

    Text {
        id: m_text_icon
        anchors.centerIn: parent
        transformOrigin: Item.Center

        font.family: root.fill ? MD.Token.font.icon_fill_family : MD.Token.font.icon_family
        font.weight: root.weight
        font.variableAxes: {
            if (!MD.Token.font.is_vf_icon) {
                return {};
            }
            return {
                "FILL": root._fillSeg
            };
        }
        font.pixelSize: Math.max(1, Math.round(root.size * root._renderScale))
        scale: 1.0 / root._renderScale

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: root.name
        color: root.color
        lineHeight: font.pixelSize
        lineHeightMode: Text.FixedHeight
    }
}
