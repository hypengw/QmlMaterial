import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Flickable {
    id: root

    contentHeight: contentItem ? contentItem.childrenRect.height : 0
    contentWidth: width - rightMargin - leftMargin

    T.ScrollBar.vertical: MD.ScrollBar {}
}
