import QtQuick
import QtQml
import Qcm.Material as MD

MD.Rectangle {
    id: root
    required property Flickable view
    property bool includeHorizontalMargins: true
    property int excludeBegin: 0
    property int excludeEnd: 0
    property int topMargin: view?.topMargin ?? 0
    property int bottomMargin: view?.bottomMargin ?? 0
    property int leftMargin: view?.leftMargin ?? 0
    property int rightMargin: view?.rightMargin ?? 0
    property int contentWidth: view?.contentItem.width ?? 0
    property int contentHeight: view?.contentItem.height ?? 0

    x: (view?.originX ?? 0) + (includeHorizontalMargins ? 0 : (view?.leftMargin ?? 0))
    y: view ? -view.contentY + view.originY + excludeBegin - topMargin : 0

    color: MD.MProp.backgroundColor

    implicitWidth: contentWidth + (includeHorizontalMargins ? leftMargin + rightMargin : 0)
    implicitHeight: Math.max(0, contentHeight - excludeBegin - excludeEnd + topMargin + bottomMargin)
}
