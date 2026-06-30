import QtQuick
import Qcm.Material as MD

TableView {
    id: control

    property int radius: MD.Token.shape.corner.extra_large
    property bool hasHeader: false
    readonly property real effectiveRadius: {
        const _contentSize = contentWidth + contentHeight;
        let out = radius;
        if (rows > 0) {
            const lastRowHeight = rowHeight(rows - 1);
            if (lastRowHeight > 0)
                out = Math.min(out, lastRowHeight / 2);
            if (!hasHeader) {
                const firstRowHeight = rowHeight(0);
                if (firstRowHeight > 0)
                    out = Math.min(out, firstRowHeight / 2);
            }
        }
        if (columns > 0) {
            const firstColumnWidth = columnWidth(0);
            const lastColumnWidth = columnWidth(columns - 1);
            if (firstColumnWidth > 0)
                out = Math.min(out, firstColumnWidth / 2);
            if (lastColumnWidth > 0)
                out = Math.min(out, lastColumnWidth / 2);
        }
        return out;
    }
    property MD.corners corners: hasHeader ? MD.Util.corners(0, 0, effectiveRadius, effectiveRadius) : MD.Util.corners(effectiveRadius)
    property color outlineColor: MD.Token.color.outline_variant
    property int outlineWidth: 1
    readonly property int hoveredRow: m_hover.hovered ? lastHoveredRow : -1
    property int lastHoveredRow: -1
    signal cellHovered(int row, int column)

    delegate: MD.TableViewDelegate { }

    clip: true
    synchronousDrag: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    rebound: Transition {}

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    onCellHovered: function (row, column) {
        lastHoveredRow = row;
    }
    selectionBehavior: TableView.SelectRows
    HoverHandler {
        id: m_hover
    }

    MD.Rectangle {
        parent: control
        anchors.fill: parent
        z: 1000
        corners: control.corners
        color: "transparent"
        border.width: control.outlineWidth
        border.color: control.outlineColor
        enabled: false
    }
}
