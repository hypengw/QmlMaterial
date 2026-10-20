pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.SelectionButtonBase {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: leftMargin
    rightInset: rightMargin

    verticalPadding: 8
    leftPadding: 16 + leftMargin
    rightPadding: (trailing ? 16 : 24) + rightMargin
    spacing: 0
    clip: false

    icon.width: 24
    icon.height: 24
    icon.color: control.MD.MProp.color.on_background

    // Delegates opt into model injection with `required index` / `required model`.
    property int index: -1
    property var model: null

    property int leftMargin: 0
    property int rightMargin: 0

    property int count: (ListView.view?.count ?? GridView.view?.count) ?? 0

    readonly property bool prevSameSection: ListView.view !== null && ListView.section == ListView.previousSection
    readonly property bool nextSameSection: ListView.view !== null && ListView.section == ListView.nextSection
    property bool showDivider: index >= 0 && index + 1 < count && (!ListView.view || nextSameSection)

    property MD.ListItemStyle mdState: MD.ListItemStyle {
        selected: control.selected
        item: control
    }

    property string supportText
    property Component supporting: null
    readonly property bool hasSupporting: supporting !== null || supportText.length > 0
    property int elide: Text.ElideRight
    property int wrapMode: Text.NoWrap
    property int maximumLineCount: 1

    property Component leader: MD.IconView {
        icon: control.icon
    }

    property Component trailing: null
    property Component below: null
    property Component divider: null

    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)

    property int heightMode: hasSupporting ? MD.Enum.ListItemTwoLine : MD.Enum.ListItemOneLine

    font.capitalization: Font.MixedCase

    contentItem: mdState.contentItem
    background: mdState.background
}
