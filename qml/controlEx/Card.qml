import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.CardElevated
    property MD.StateCard mdState: MD.StateCard {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 0
    horizontalPadding: 16

    contentItem: Item {}

    background: MD.ElevationRectangleBorder {
        implicitWidth: 64
        implicitHeight: 64

        radius: 12
        color: control.mdState.backgroundColor

        border.width: control.type == MD.Enum.CardOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline

        elevationVisible: elevation && color.a > 0 && !control.flat
        elevation: control.mdState.elevation
    }

    Binding {
        control.contentItem.z: -1
        control.background.z: -2
        when: control.contentItem
    }
    MD.Ripple2 {
        anchors.fill: parent
        radius: 12
        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: control.mdState.stateLayerOpacity
        color: control.mdState.stateLayerColor
    }

    MD.StateHolder {
        state: control.mdState
    }
}
