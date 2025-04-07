import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TabButton {
    id: control

    property int type: MD.Enum.PrimaryTab
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    property alias mdState: m_sh.state

    // use checked instead
    // property bool active: T.TabBar.index === T.TabBar.tabBar.currentIndex

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    spacing: 8

    leftPadding: 12
    rightPadding: 12

    icon.width: 24
    icon.height: 24

    contentItem: MD.IconLabel {
        font: control.font
        text: control.text
        typescale: MD.Token.typescale.title_small
        color: control.mdState.textColor
        icon_style: control.iconStyle

        icon_name: control.icon.name
        icon_size: control.icon.width
        opacity: control.mdState.contentOpacity
    }

    background: MD.Ripple2 {
        implicitHeight: 48

        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: control.mdState.stateLayerOpacity
        color: control.mdState.stateLayerColor
        // opacity: control.mdState.backgroundOpacity
    }

    MD.StateHolder {
        id: m_sh
        state: MD.StateTabButton {
            item: control
        }
    }
}
