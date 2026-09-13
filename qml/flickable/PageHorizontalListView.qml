pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.HorizontalListView {
    id: root

    property MD.PageContext pageContext: MD.MProp.page
    property int pageMarginEdges: Qt.LeftEdge | Qt.RightEdge
    property real extraLeftMargin: 0
    property real extraRightMargin: 0
    property real extraTopMargin: 0
    property real extraBottomMargin: 0

    leftMargin: extraLeftMargin + ((pageMarginEdges & Qt.LeftEdge) ? (pageContext?.leftMargin ?? 0) : 0)
    rightMargin: extraRightMargin + ((pageMarginEdges & Qt.RightEdge) ? (pageContext?.rightMargin ?? 0) : 0)
    topMargin: extraTopMargin + ((pageMarginEdges & Qt.TopEdge) ? (pageContext?.topMargin ?? 0) : 0)
    bottomMargin: extraBottomMargin + ((pageMarginEdges & Qt.BottomEdge) ? (pageContext?.bottomMargin ?? 0) : 0)
}
