pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.VerticalLazyGrid {
    id: root
    property MD.PageContext pageContext: MD.MProp.page
    property int pageMarginEdges: Qt.LeftEdge | Qt.RightEdge | Qt.TopEdge | Qt.BottomEdge
    leftMargin: extraLeftMargin + ((pageMarginEdges & Qt.LeftEdge) ? (pageContext?.leftMargin ?? 0) : 0)
    rightMargin: extraRightMargin + ((pageMarginEdges & Qt.RightEdge) ? (pageContext?.rightMargin ?? 0) : 0)
    topMargin: extraTopMargin + (headerItem?.height ?? 0) + ((pageMarginEdges & Qt.TopEdge) ? (pageContext?.topMargin ?? 0) : 0)
    bottomMargin: extraBottomMargin + (footerItem?.height ?? 0) + ((pageMarginEdges & Qt.BottomEdge) ? (pageContext?.bottomMargin ?? 0) : 0)
}
