import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Dialog {
    id: control

    property int titleCapitalization: Font.Capitalize
    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    verticalPadding: 24

    modal: true
    clip: false

    enter: Transition {
        // grow_fade_in
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    exit: Transition {
        // shrink_fade_out
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    background: Rectangle {
        // FullScale doesn't make sense for Dialog.
        radius: MD.Token.shape.corner.extra_large
        color: control.mdState.backgroundColor

        layer.enabled: control.mdState.elevation
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.mdState.elevation
            roundedScale: control.background.radius
        }
    }

    header: MD.Control {
        topPadding: 24
        horizontalPadding: 24
        contentItem: ColumnLayout {
            MD.Text {
                visible: control.title
                text: control.title
                typescale: MD.Token.typescale.headline_small
                color: item_state.ctx.color.on_surface
                font.capitalization: control.titleCapitalization
            }
        }
    }

    footer: Item {
    }

    //    DialogButtonBox {
    //        visible: count > 0
    //    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(item_state.ctx.color.scrim, 0.32)
        Behavior on opacity  {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(item_state.ctx.color.scrim, 0.32)
        Behavior on opacity  {
            NumberAnimation {
                duration: 150
            }
        }
    }

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level3
        textColor: item_state.ctx.color.primary
        backgroundColor: item_state.ctx.color.surface_container_high
        supportTextColor: item_state.ctx.color.on_surface_variant
    }
}
