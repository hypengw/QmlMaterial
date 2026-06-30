pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.HeaderViewDelegate {
    id: control

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8

    required property int row
    required property int column
    readonly property int section: Math.max(row, column)
    readonly property string textRole: control.headerView?.textRole ?? ""
    readonly property int rows: {
        const syncRows = control.headerView?.syncView?.rows ?? 0;
        if (syncRows > 0)
            return syncRows;
        const headerModel = control.headerView?.model;
        if (Array.isArray(headerModel))
            return headerModel.length;
        if (typeof headerModel?.length === "number")
            return headerModel.length;
        const headerRows = control.headerView?.rows ?? 0;
        if (headerRows > 0)
            return headerRows;
        return control.headerView?.syncView?.rows ?? 0;
    }
    readonly property var displayText: {
        if (control.model === undefined || control.model === null)
            return "";
        if (control.textRole.length > 0 && control.model[control.textRole] !== undefined)
            return control.model[control.textRole];
        if (control.model.display !== undefined)
            return control.model.display;
        if (control.model.modelData !== undefined)
            return control.model.modelData;
        return control.model;
    }
    highlighted: control.selected
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        implicitWidth: 56
        implicitHeight: 44
        color: control.selected ? MD.Token.color.surface_container_highest : MD.Token.color.surface

        MD.Ripple {
            anchors.fill: parent
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.pressed ? MD.Token.state.pressed.state_layer_opacity : (control.hovered ? MD.Token.state.hover.state_layer_opacity : 0)
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
        color: control.hovered ? MD.Token.color.on_surface : MD.Token.color.on_surface_variant
    }
}
