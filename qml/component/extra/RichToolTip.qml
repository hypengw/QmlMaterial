pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

/**
 * @brief Material Design 3 rich tooltip with optional subhead and actions
 * @ingroup component
 */
T.ToolTip {
    id: control

    property MD.MState mdState: MD.MState {
        target: control

        elevation: MD.Token.elevation.level2
        textColor: ctx.color.on_surface
        backgroundColor: ctx.color.surface_container
        supportTextColor: ctx.color.on_surface_variant
    }
    property MD.typescale subheadTypescale: MD.Token.typescale.title_small
    property MD.typescale typescale: MD.Token.typescale.body_medium
    property int radius: MD.Token.shape.corner.medium

    property string subhead
    property Item actionItem: null

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 4

    margins: 8
    padding: 16

    delay: 0
    timeout: -1

    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside | T.Popup.CloseOnReleaseOutside

    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    enter: Transition {
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing: MD.Token.easing.standard
            duration: MD.Token.duration.medium2
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "scale"
            to: 0.9
            easing: MD.Token.easing.emphasized_accelerate
            duration: MD.Token.duration.short3
        }
        NumberAnimation {
            property: "opacity"
            to: 0.0
            easing: MD.Token.easing.standard
            duration: MD.Token.duration.short3
        }
    }

    contentItem: Item {
        implicitWidth: Math.max(m_subhead.implicitWidth, m_text.implicitWidth,
                                m_action_holder.implicitWidth)
        implicitHeight: m_text.y + m_text.implicitHeight
                        + (m_action_holder.visible ? 8 + m_action_holder.implicitHeight : 0)

        MD.Text {
            id: m_subhead
            visible: control.subhead.length > 0
            anchors.left: parent.left
            anchors.right: parent.right
            text: control.subhead
            typescale: control.subheadTypescale
            color: control.mdState.textColor
            wrapMode: Text.Wrap
            elide: Text.ElideNone
        }

        MD.Text {
            id: m_text
            anchors.left: parent.left
            anchors.right: parent.right
            y: m_subhead.visible ? m_subhead.height + 4 : 0
            text: control.text
            typescale: control.typescale
            color: control.mdState.supportTextColor
            wrapMode: Text.Wrap
            elide: Text.ElideNone
        }

        MD.ItemHolder {
            id: m_action_holder
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: m_text.bottom
            anchors.topMargin: 8
            visible: control.actionItem !== null
            item: control.actionItem
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 40
        radius: control.radius
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }
}
