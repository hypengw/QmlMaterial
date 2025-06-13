import QtQuick

TableView {
    readonly property int hoveredRow: m_hover.hovered ? lastHoveredRow : -1
    property int lastHoveredRow: -1
    selectionBehavior: TableView.SelectRows
    HoverHandler {
        id: m_hover
    }
}
