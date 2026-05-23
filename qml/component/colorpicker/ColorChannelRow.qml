pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

RowLayout {
    id: root

    enum Channel { Hue, Saturation, Value, Red, Green, Blue, Alpha }

    property int channel: ColorChannelRow.Channel.Hue
    property color contextColor: "black"
    property real value: 0
    property real from: 0
    property real to: 255
    property real step: 1
    property string label

    signal moved(real v)

    spacing: 8

    function _hsvHue(c) { return c.hsvHue >= 0 ? c.hsvHue : 0; }
    function _colorFrom() {
        const c = root.contextColor;
        switch (root.channel) {
        case ColorChannelRow.Channel.Saturation:
            return Qt.hsva(_hsvHue(c), 0, c.hsvValue, 1);
        case ColorChannelRow.Channel.Value:
            return Qt.hsva(_hsvHue(c), c.hsvSaturation, 0, 1);
        case ColorChannelRow.Channel.Red:
            return Qt.rgba(0, c.g, c.b, 1);
        case ColorChannelRow.Channel.Green:
            return Qt.rgba(c.r, 0, c.b, 1);
        case ColorChannelRow.Channel.Blue:
            return Qt.rgba(c.r, c.g, 0, 1);
        case ColorChannelRow.Channel.Alpha:
            return Qt.rgba(c.r, c.g, c.b, 0);
        }
        return "black";
    }
    function _colorTo() {
        const c = root.contextColor;
        switch (root.channel) {
        case ColorChannelRow.Channel.Saturation:
            return Qt.hsva(_hsvHue(c), 1, c.hsvValue, 1);
        case ColorChannelRow.Channel.Value:
            return Qt.hsva(_hsvHue(c), c.hsvSaturation, 1, 1);
        case ColorChannelRow.Channel.Red:
            return Qt.rgba(1, c.g, c.b, 1);
        case ColorChannelRow.Channel.Green:
            return Qt.rgba(c.r, 1, c.b, 1);
        case ColorChannelRow.Channel.Blue:
            return Qt.rgba(c.r, c.g, 1, 1);
        case ColorChannelRow.Channel.Alpha:
            return Qt.rgba(c.r, c.g, c.b, 1);
        }
        return "white";
    }

    MD.Text {
        Layout.preferredWidth: 14
        text: root.label
        typescale: MD.Token.typescale.label_medium
        color: MD.MProp.color.on_surface_variant
        horizontalAlignment: Text.AlignHCenter
    }

    Loader {
        Layout.fillWidth: true
        Layout.preferredHeight: 20
        sourceComponent: root.channel === ColorChannelRow.Channel.Hue ? m_hueSlider : m_channelSlider
    }

    MD.StepperInput {
        Layout.preferredWidth: 60
        Layout.preferredHeight: 24
        value: root.value
        from: root.from
        to: root.to
        step: root.step
        wrap: root.channel === ColorChannelRow.Channel.Hue
        onMoved: function (v) { root.moved(v); }
    }

    Component {
        id: m_hueSlider
        MD.HueChannelSlider {
            value: root.value
            from: root.from
            to: root.to
            onMoved: function (v) { root.moved(v); }
        }
    }
    Component {
        id: m_channelSlider
        MD.ChannelSlider {
            value: root.value
            from: root.from
            to: root.to
            colorFrom: root._colorFrom()
            colorTo: root._colorTo()
            showChecker: root.channel === ColorChannelRow.Channel.Alpha
            indicatorColor: {
                if (root.channel === ColorChannelRow.Channel.Alpha)
                    return Qt.rgba(root.contextColor.r, root.contextColor.g, root.contextColor.b, root.value / (root.to - root.from));
                return root._colorTo();
            }
            onMoved: function (v) { root.moved(v); }
        }
    }
}
