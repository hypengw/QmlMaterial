pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.MenuItemBase {
    id: control

    property alias arrow: defaultStyle.arrow
    property bool selected: false
    property bool highlighted: false
    property int busy: MD.Enum.Idle
    property real progress: 0
    property MD.MenuItemStyle mdState: MD.MenuItemStyle {
        id: defaultStyle
        item: control
    }
    Binding {
        control.mdState.selected: control.selected
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 0
    spacing: 16

    icon.width: 24
    icon.height: 24
    icon.color: control.leadingIconColor
    icon.fill: control.selected

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.MixedCase
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: mdState.contentItem
    background: mdState.background

    property color leadingIconColor: control.mdState.leadingIconColor
    property color trailingIconColor: control.mdState.trailingIconColor
}
