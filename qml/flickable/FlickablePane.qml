import QtQuick
import QtQml
import Qcm.Material as MD

MD.Rectangle {
    id: root
    required property Flickable view
    property int excludeBegin: 0
    property int excludeEnd: 0
    property int topMargin: view.topMargin
    property int bottomMargin: view.bottomMargin
    property int leftMargin: view.leftMargin
    property int rightMargin: view.rightMargin
    property int contentWidth: view.contentItem.width
    property int contentHeight: view.contentItem.height

    x: view.originX 
    y: -view.contentY + view.originY + excludeBegin - topMargin

    color: MD.MatProp.backgroundColor

    implicitWidth: contentWidth + leftMargin + rightMargin
    implicitHeight: contentHeight - excludeBegin + (topMargin + bottomMargin)
}
