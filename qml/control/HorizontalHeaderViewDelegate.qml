pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ItemDelegate {
    id: control

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8

    required property int column
    required property int row
    required property bool selected

    readonly property MD.HorizontalHeaderViewBase headerView: TableView.view as MD.HorizontalHeaderViewBase
    readonly property int section: headerView?.visualSections[column] ?? column
    readonly property string textRole: control.headerView?.textRole ?? ""
    readonly property int columns: headerView?.sectionCount ?? 0
    readonly property int radius: (control.headerView?.syncView as QtObject)?.effectiveRadius ?? 0
    readonly property MD.corners corners: MD.Util.corners(section === 0 ? radius : 0, section + 1 === columns ? radius : 0, 0, 0)
    readonly property string displayText: headerView?.headerTexts[column] ?? ""
    highlighted: control.selected

    background: MD.Rectangle {
        implicitWidth: 64
        implicitHeight: 48
        corners: control.corners
        color: control.selected ? MD.Token.color.surface_container_highest : MD.Token.color.surface_container

        MD.Ripple {
            anchors.fill: parent
            corners: control.corners
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.pressed ? MD.Token.state.pressed.state_layer_opacity : (control.hovered ? MD.Token.state.hover.state_layer_opacity : 0)
            color: MD.Token.color.on_surface
        }

        MD.Divider {
            anchors.bottom: parent.bottom
            width: parent.width
        }

        MD.Divider {
            anchors.right: parent.right
            height: parent.height
            orientation: Qt.Vertical
            visible: control.section + 1 !== control.columns
        }
    }

    contentItem: MD.Label {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: control.displayText ?? ""
        typescale: MD.Token.typescale.title_small
        color: control.hovered ? MD.Token.color.on_surface : MD.Token.color.on_surface_variant
    }
}
