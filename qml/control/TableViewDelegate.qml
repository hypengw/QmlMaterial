pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TableViewDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8

    required property int column
    required property int row
    required property var model
    readonly property bool rowHovered: hovered || ((TableView.view as MD.TableView)?.hoveredRow ?? -1) === row
    property int rows: TableView.view?.rows ?? 0
    property int columns: TableView.view?.columns ?? 0
    readonly property MD.TableView mdTableView: TableView.view as MD.TableView
    property MD.StateTableViewDelegate mdState: MD.StateTableViewDelegate {
        item: control
    }
    property int radius: mdTableView?.effectiveRadius ?? 0
    property MD.corners corners: mdTableView?.hasHeader
                                 ? MD.Util.tableWithHeaderCorners(row, column, rows, columns, radius)
                                 : MD.Util.tableCorners(row, column, rows, columns, radius)

    highlighted: selected

    onHoveredChanged: MD.Util.cellHoveredOn(TableView.view, hovered, row, column)

    contentItem: MD.Label {
        clip: false
        text: control.model.display ?? ""
        elide: Text.ElideRight
        typescale: MD.Token.typescale.body_medium
        verticalAlignment: Text.AlignVCenter
        color: control.mdState.textColor
        opacity: control.mdState.contentOpacity
        visible: !control.editing
    }

    background: MD.ElevationRectangle {
        implicitWidth: 64
        implicitHeight: 44

        opacity: control.mdState.backgroundOpacity

        corners: control.corners

        color: control.mdState.backgroundColor

        elevationVisible: elevation && color.a > 0
        elevation: control.mdState.elevation

        MD.Ripple {
            id: m_ripple
            corners: control.corners
            width: parent.width
            height: parent.height
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }

        MD.Divider {
            anchors.bottom: parent.bottom
            width: parent.width
            color: control.mdState.outlineColor
            visible: control.row + 1 !== control.rows
        }

        MD.Divider {
            anchors.right: parent.right
            height: parent.height
            orientation: Qt.Vertical
            color: control.mdState.outlineColor
            visible: control.column + 1 !== control.columns
        }
    }

    // TableView.editDelegate: FocusScope {
    //     width: parent.width
    //     height: parent.height

    //     TableView.onCommit: {
    //         let model = control.tableView.model;
    //         if (!model)
    //             return;
    //         let succeed = false;
    //         const index = model.index(control.row, control.column);
    //         succeed = model.setData(index, textField.text, Qt.EditRole);
    //         if (!succeed)
    //             console.warn("The model does not allow setting the EditRole data.");
    //     }

    //     Component.onCompleted: textField.selectAll()

    //     TextField {
    //         id: textField
    //         anchors.fill: parent
    //         text: control.model.edit ?? control.model.display ?? ""
    //         focus: true
    //     }
    // }
}
