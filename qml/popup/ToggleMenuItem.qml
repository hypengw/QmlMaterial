pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ToggleMenuItemBase {
    id: control

    property alias arrow: defaultStyle.arrow
    property alias supportText: defaultStyle.supportText
    property alias trailingText: defaultStyle.trailingText
    property alias trailingIcon: defaultStyle.trailingIcon
    property alias badge: defaultStyle.badge
    readonly property bool selected: checked
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

    padding: MD.MenuMetrics.horizontalPadding + (mdState.expressive ? MD.MenuMetrics.horizontalInset : 0)
    verticalPadding: 0
    spacing: mdState.expressive ? MD.MenuMetrics.spacing : MD.MenuMetrics.baselineSpacing
    hoverEnabled: true

    icon.width: mdState.expressive ? MD.MenuMetrics.iconSize : MD.MenuMetrics.baselineIconSize
    icon.height: icon.width
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
