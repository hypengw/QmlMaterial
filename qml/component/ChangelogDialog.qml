pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Dialog {
    id: control

    property alias source: changelog.source
    readonly property alias status: changelog.status
    readonly property alias errorString: changelog.errorString
    readonly property alias releaseCount: changelog.releaseCount

    width: Math.min(448, parent ? parent.width - 48 : 448)
    height: Math.min(640, parent ? parent.height - 48 : 640)
    horizontalPadding: 0
    standardButtons: T.DialogButtonBox.Close

    contentItem: MD.Changelog {
        id: changelog
        implicitWidth: 384
        implicitHeight: 480
    }
}
