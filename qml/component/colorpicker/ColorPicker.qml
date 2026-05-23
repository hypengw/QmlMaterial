pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control

    property color color
    property bool showAlpha: true
    property bool showHeader: true

    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    horizontalPadding: 12

    background: null

    // Single source of truth — never bound as destination from UI elements.
    QtObject {
        id: m_state
        property real h: 0
        property real s: 1
        property real v: 1
        property real a: 1
    }
    QtObject {
        id: m_guard
        property bool internal: false
    }
    property int _mode: 0 // 0 HSV, 1 RGB

    function _emit() {
        m_guard.internal = true;
        control.color = Qt.hsva(m_state.h, m_state.s, m_state.v, m_state.a);
        m_guard.internal = false;
    }
    function _writeFromColor(c) {
        const h = c.hsvHue >= 0 ? c.hsvHue : m_state.h;
        m_state.h = h;
        m_state.s = c.hsvSaturation;
        m_state.v = c.hsvValue;
        m_state.a = c.a;
    }

    function setHue(h)        { m_state.h = h; _emit(); }
    function setSaturation(s) { m_state.s = s; _emit(); }
    function setValue_(v)     { m_state.v = v; _emit(); }
    function setSV(s, v)      { m_state.s = s; m_state.v = v; _emit(); }
    function setAlpha(a)      { m_state.a = a; _emit(); }
    function setR(r) {
        const c = Qt.rgba(r, control.color.g, control.color.b, m_state.a);
        _writeFromColor(c); _emit();
    }
    function setG(g) {
        const c = Qt.rgba(control.color.r, g, control.color.b, m_state.a);
        _writeFromColor(c); _emit();
    }
    function setB(b) {
        const c = Qt.rgba(control.color.r, control.color.g, b, m_state.a);
        _writeFromColor(c); _emit();
    }
    function setColor(c) { _writeFromColor(c); _emit(); }

    Component.onCompleted: _writeFromColor(control.color)
    onColorChanged: {
        if (m_guard.internal) return;
        _writeFromColor(control.color);
    }

    function _hex2(v) {
        const s = Math.round(v).toString(16);
        return s.length === 1 ? "0" + s : s;
    }
    function _hexOf(c) {
        const rgb = "#" + _hex2(c.r * 255) + _hex2(c.g * 255) + _hex2(c.b * 255);
        return showAlpha ? rgb + _hex2(c.a * 255) : rgb;
    }

    contentItem: ColumnLayout {
        spacing: 12

        MD.ColorWheel {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 240
            implicitHeight: 240
            visible: control.showHeader
            hue: m_state.h
            saturation: m_state.s
            value: m_state.v
            onUserHueChanged: function (h) { control.setHue(h); }
            onUserSvChanged: function (s, v) { control.setSV(s, v); }
        }

        // Chip + Segmented mode toggle on the same row.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            MD.InputChip {
                Layout.alignment: Qt.AlignVCenter
                text: control._hexOf(control.color).toUpperCase()
                leadingItem: Rectangle {
                    width: 20
                    height: 20
                    radius: 4
                    color: control.color
                    border.width: 1
                    border.color: MD.Util.transparent(MD.MProp.color.on_surface, 0.2)
                }
            }
            Item { Layout.fillWidth: true }
            MD.SegmentedButtonGroup {
                Layout.alignment: Qt.AlignVCenter
                size: MD.Enum.XS
                MD.SegmentedButton {
                    text: "HSV"
                    checked: control._mode === 0
                    onClicked: control._mode = 0
                }
                MD.SegmentedButton {
                    text: "RGB"
                    checked: control._mode === 1
                    onClicked: control._mode = 1
                }
            }
        }

        // Channel rows
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 0
                channel: MD.ColorChannelRow.Channel.Hue
                contextColor: control.color
                from: 0; to: 360; step: 1
                value: Math.round(m_state.h * 360)
                label: "H"
                onMoved: function (v) { control.setHue(v / 360); }
            }
            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 0
                channel: MD.ColorChannelRow.Channel.Saturation
                contextColor: control.color
                from: 0; to: 100; step: 1
                value: Math.round(m_state.s * 100)
                label: "S"
                onMoved: function (v) { control.setSaturation(v / 100); }
            }
            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 0
                channel: MD.ColorChannelRow.Channel.Value
                contextColor: control.color
                from: 0; to: 100; step: 1
                value: Math.round(m_state.v * 100)
                label: "V"
                onMoved: function (v) { control.setValue_(v / 100); }
            }

            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 1
                channel: MD.ColorChannelRow.Channel.Red
                contextColor: control.color
                from: 0; to: 255; step: 1
                value: Math.round(control.color.r * 255)
                label: "R"
                onMoved: function (v) { control.setR(v / 255); }
            }
            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 1
                channel: MD.ColorChannelRow.Channel.Green
                contextColor: control.color
                from: 0; to: 255; step: 1
                value: Math.round(control.color.g * 255)
                label: "G"
                onMoved: function (v) { control.setG(v / 255); }
            }
            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control._mode === 1
                channel: MD.ColorChannelRow.Channel.Blue
                contextColor: control.color
                from: 0; to: 255; step: 1
                value: Math.round(control.color.b * 255)
                label: "B"
                onMoved: function (v) { control.setB(v / 255); }
            }

            MD.ColorChannelRow {
                Layout.fillWidth: true
                visible: control.showAlpha
                channel: MD.ColorChannelRow.Channel.Alpha
                contextColor: control.color
                from: 0; to: 255; step: 1
                value: Math.round(m_state.a * 255)
                label: "A"
                onMoved: function (v) { control.setAlpha(v / 255); }
            }
        }
    }
}
