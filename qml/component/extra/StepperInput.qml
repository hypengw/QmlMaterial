pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root

    property real value: 0
    property real from: 0
    property real to: 100
    property real step: 1
    property bool wrap: false
    property int decimals: 0
    property int valueWidth: 28

    signal moved(real v)

    implicitWidth: m_valueText.implicitWidth + valueWidth + 22
    implicitHeight: 24

    color: MD.MProp.color.surface_container_highest
    radius: 4

    function _format(v) {
        return Number(v).toFixed(decimals);
    }
    function _commit(v) {
        if (wrap && from !== to) {
            const span = to - from;
            while (v < from) v += span;
            while (v > to) v -= span;
        } else {
            v = Math.max(from, Math.min(to, v));
        }
        root.moved(v);
    }

    TextInput {
        id: m_input
        anchors.left: parent.left
        anchors.right: m_stepper.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 6
        anchors.rightMargin: 2
        text: root._format(root.value)
        horizontalAlignment: TextInput.AlignRight
        verticalAlignment: TextInput.AlignVCenter
        color: MD.MProp.color.on_surface
        font.pixelSize: MD.Token.typescale.label_small.size
        selectByMouse: true
        clip: true
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        validator: DoubleValidator {
            bottom: Math.floor(root.from)
            top: Math.ceil(root.to)
            decimals: root.decimals
        }
        onEditingFinished: {
            const v = parseFloat(m_input.text);
            if (!isNaN(v)) root._commit(v);
            else m_input.text = root._format(root.value);
        }

        MD.Text {
            id: m_valueText
            visible: false
            text: "999"
            font: m_input.font
        }
    }

    MD.NumberStepper {
        id: m_stepper
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 2
        width: 14
        height: parent.height - 4
        value: root.value
        from: root.from
        to: root.to
        step: root.step
        wrap: root.wrap
        onStepped: function (v) { root._commit(v); }
    }
}
