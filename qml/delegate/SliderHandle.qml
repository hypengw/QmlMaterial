import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: horizontal ? handleWidth : handleHeight
    implicitHeight: horizontal ? handleHeight : handleWidth

    property real value: 0
    property int labelBehavior: MD.Enum.SliderLabelFloating
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    property int handleWidth: MD.Token.slider.handle_width
    property int handleHeight: MD.Token.slider.handle_height
    property bool horizontal: false
    property int handleLineWidth: 4

    readonly property var control: parent

    readonly property bool __valueIndicatorActive: {
        if (root.labelBehavior === MD.Enum.SliderLabelGone)
            return false;
        if (root.labelBehavior === MD.Enum.SliderLabelVisible)
            return root.control ? root.control.enabled : false;
        return root.handlePressed || root.handleHasFocus || root.handleHovered;
    }

    // The value indicator (bubble)
    MD.Control {
        id: valueIndicator
        y: {
            if (!root.horizontal)
                return (parent.height - height) / 2;
            if (root.labelBehavior === MD.Enum.SliderLabelWithinBounds) {
                const above = -height - 4;
                const below = parent.height + 4;
                return above >= 0 ? above : below;
            }
            return -height - 4;
        }
        x: {
            if (root.horizontal)
                return (parent.width - width) / 2;
            if (root.labelBehavior === MD.Enum.SliderLabelWithinBounds) {
                const left = -width - 4;
                const right = parent.width + 4;
                return left >= 0 ? left : right;
            }
            return -width - 4;
        }

        visible: root.__valueIndicatorActive
        opacity: visible ? 1 : 0
        scale: visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short2
            }
        }
        Behavior on scale {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }

        contentItem: Item {
            implicitHeight: children[0].implicitHeight
            implicitWidth: children[0].implicitWidth
            MD.Text {
                anchors.centerIn: parent
                text: Math.round(root.value)
                typescale: MD.Token.typescale.label_medium
                color: root.control ? root.control.mdState.ctx.color.inverse_on_surface : "transparent"
            }
        }
        background: MD.ElevationRectangle {
            implicitWidth: 32
            implicitHeight: 32
            radius: 16
            color: root.control ? root.control.mdState.ctx.color.inverse_surface : "transparent"
            elevation: MD.Token.elevation.level2
        }
    }

    // The vertical/horizontal line handle (separator)
    Rectangle {
        id: handleRect
        anchors.centerIn: parent

        width: root.horizontal ? root.handleLineWidth : root.handleHeight
        height: root.horizontal ? root.handleHeight : root.handleLineWidth

        radius: MD.Token.slider.track_inside_corner
        color: root.control ? root.control.mdState.backgroundColor : "transparent"
    }

    // The "pill" outline handle shown when pressed/focused to emphasize
    Rectangle {
        id: pillHandle
        anchors.centerIn: parent
        width: (root.horizontal ? root.handleLineWidth : root.handleHeight) + 12
        height: (root.horizontal ? root.handleHeight : root.handleLineWidth) + 12
        radius: ((root.horizontal ? root.handleLineWidth : root.handleLineWidth) + 12) / 2
        color: "transparent"
        border.color: root.control ? root.control.mdState.backgroundColor : "transparent"
        border.width: 4
        visible: root.handleHasFocus

        opacity: visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short2
            }
        }
    }
}
