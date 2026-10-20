pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ItemDelegate {
    id: control

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8

    required property int row
    required property int column
    required property bool selected

    readonly property MD.VerticalHeaderViewBase headerView: TableView.view as MD.VerticalHeaderViewBase
    readonly property int section: headerView?.visualSections[row] ?? row
    readonly property string textRole: control.headerView?.textRole ?? ""
    readonly property int rows: headerView?.sectionCount ?? 0
    readonly property string displayText: headerView?.headerTexts[row] ?? ""
    highlighted: control.selected

    MD.InteractionObserver {
        id: feedback
        source: control.interactionSource
    }

    background: Rectangle {
        implicitWidth: 56
        implicitHeight: 44
        color: control.selected ? MD.Token.color.surface_container_highest : MD.Token.color.surface

        MD.Ripple {
            anchors.fill: parent
            interactionSource: control.interactionSource
            color: MD.Token.color.on_surface
        }

        MD.Divider {
            anchors.right: parent.right
            height: parent.height
            orientation: Qt.Vertical
        }

        MD.Divider {
            anchors.bottom: parent.bottom
            width: parent.width
            visible: control.section + 1 !== control.rows
        }
    }

    contentItem: MD.Label {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: control.displayText ?? ""
        typescale: MD.Token.typescale.title_small
        color: feedback.hovered ? MD.Token.color.on_surface : MD.Token.color.on_surface_variant
    }
}
