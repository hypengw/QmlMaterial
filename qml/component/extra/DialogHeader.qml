pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Control {
    id: control
    topPadding: 24
    horizontalPadding: 24

    property string title

    contentItem: Item {
        implicitHeight: children[0].implicitHeight
        implicitWidth: children[0].implicitWidth

        MD.Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: control.title
            text: control.title
            typescale: MD.Token.typescale.headline_small
            color: control.MD.MProp.color.on_surface
        }
    }
}
