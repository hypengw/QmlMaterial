pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root

    // Two modes:
    // - overlay (preferred): place Badge inside the control
    // - wrapper: place the control inside Badge
    default property alias content: contentHolder.data

    property Item target: null

    property string text: ""
    property int count: -1
    property int maxCount: 99
    property bool showZero: false

    property bool isLabelVisible: true

    property color backgroundColor: MD.Token.color.error
    property color textColor: MD.Token.color.on_error

    property int labelYOffset: 0

    property int alignment: Qt.AlignRight | Qt.AlignTop
    property point offset: Qt.point(MD.Token.badge.def.offset_x, MD.Token.badge.def.offset_y)

    readonly property bool _wrapMode: contentHolder.children.length > 0
    readonly property Item _autoTarget: {
        if (root.target)
            return root.target
        if (root._wrapMode)
            return contentHolder.children[0]
        return root.parent
    }

    implicitWidth: _wrapMode ? contentHolder.implicitWidth : (_autoTarget ? _autoTarget.implicitWidth : 0)
    implicitHeight: _wrapMode ? contentHolder.implicitHeight : (_autoTarget ? _autoTarget.implicitHeight : 0)

    width: _wrapMode ? implicitWidth : (_autoTarget ? _autoTarget.width : 0)
    height: _wrapMode ? implicitHeight : (_autoTarget ? _autoTarget.height : 0)

    x: (!_wrapMode && _autoTarget && root.parent !== _autoTarget) ? _autoTarget.x : 0
    y: (!_wrapMode && _autoTarget && root.parent !== _autoTarget) ? _autoTarget.y : 0

    Item {
        id: contentHolder
        visible: root._wrapMode
        anchors.fill: parent
    }

    readonly property string _labelText: {
        if (count >= 0) {
            if (count === 0 && !showZero)
                return ""
            if (count > maxCount)
                return String(maxCount) + "+"
            return String(count)
        }
        return text
    }

    property bool dot: false

    readonly property bool _hasLabel: isLabelVisible && _labelText.length > 0
    readonly property bool _showDot: isLabelVisible && !_hasLabel && dot && (count < 0 || count > 0)

    MD.TextMetrics {
        id: labelMetrics
        typescale: MD.Token.typescale.label_small
        text: root._labelText
    }

    Rectangle {
        id: badge
        visible: root._hasLabel || root._showDot
        color: root.backgroundColor
        antialiasing: true

        readonly property bool dotMode: root._showDot
        width: dotMode
               ? MD.Token.badge.def.small_diameter
               : Math.max(MD.Token.badge.def.large_min_width,
                          labelMetrics.width + 2 * MD.Token.badge.def.large_horizontal_padding)
        height: dotMode ? width : MD.Token.badge.def.large_height
        radius: height / 2

        x: {
            const pw = root.width
            const w = badge.width
            let base = 0
            if (root.alignment & Qt.AlignHCenter)
                base = (pw - w) / 2
            else if (root.alignment & Qt.AlignRight)
                base = pw - w
            else
                base = 0
            return base + root.offset.x
        }
        y: {
            const ph = root.height
            const h = badge.height
            let base = 0
            if (root.alignment & Qt.AlignVCenter)
                base = (ph - h) / 2
            else if (root.alignment & Qt.AlignBottom)
                base = ph - h
            else
                base = 0
            return base + root.offset.y
        }

        MD.Text {
            anchors.fill: parent
            visible: root._hasLabel
            text: root._labelText
            color: root.textColor
            typescale: MD.Token.typescale.label_small
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            topPadding: root.labelYOffset
        }
    }
}

