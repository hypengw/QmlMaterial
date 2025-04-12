pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.ListView {
    id: root
    contentHeight: height - topMargin - bottomMargin
    orientation: ListView.Horizontal

    footer: MD.ListBusyFooter {
        running: root.busy
        height: ListView.view.height
        width: implicitWidth
    }
    // T.ScrollBar.horizontal: MD.ScrollBar {}
}
