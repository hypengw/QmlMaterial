import QtQuick
import QtQuick.Controls as QC
import Qcm.Material as MD

MD.Rectangle {
    id: root
    required property Flickable view
    property int excludeBegin: 0
    property int excludeEnd: 0
    property int topMargin: 8
    property int bottomMargin: 8

    x: view.originX 
    y: -view.contentY + view.originY + excludeBegin - topMargin

    color: MD.MatProp.backgroundColor
    radius: MD.Token.shape.corner.large

    implicitWidth: view.contentItem.childrenRect.width
    implicitHeight: view.contentItem.childrenRect.height - excludeBegin + (topMargin + bottomMargin)
}
