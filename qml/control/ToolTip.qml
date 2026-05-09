import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

/**
 * @brief Material Design 3 plain tooltip
 * @ingroup control
 */
T.ToolTip {
    id: control

    property MD.MState mdState: MD.MState {
        target: control

        elevation: MD.Token.elevation.level0
        textColor: ctx.color.inverse_on_surface
        backgroundColor: ctx.color.inverse_surface
    }
    property MD.typescale typescale: MD.Token.typescale.body_small
    property int radius: MD.Token.shape.corner.extra_small

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 4

    margins: 4
    padding: 4
    horizontalPadding: 8

    delay: 500

    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent

    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    enter: Transition {
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.short4
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing: MD.Token.easing.standard
            duration: MD.Token.duration.short4
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "scale"
            to: 0.8
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

    contentItem: MD.Text {
        text: control.text
        typescale: control.typescale
        color: control.mdState.textColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }

    background: MD.ElevationRectangle {
        implicitWidth: 24
        implicitHeight: 24
        radius: control.radius
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }
}
