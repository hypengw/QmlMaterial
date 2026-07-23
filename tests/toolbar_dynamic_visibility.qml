import QtQuick
import QtQuick.Layouts
import QmlMaterialTest

Item {
    id: root

    width: 280
    height: 80

    property bool linkVisible: false
    readonly property int hiddenCount: actionBarHost.hiddenCount
    readonly property real actionBarWidth: actionBarHost.width

    QtObject {
        id: linkAction
        property bool visible: root.linkVisible
        property int displayHint: 0
    }
    QtObject {
        id: infoAction
        property bool visible: true
        property int displayHint: 0
    }
    QtObject {
        id: closeAction
        property bool visible: true
        property int displayHint: 0
    }

    RowLayout {
        anchors.fill: parent

        Item {
            Layout.fillWidth: true
        }

        ToolbarDynamicHost {
            id: actionBarHost
            actions: [linkAction, infoAction, closeAction]
        }
    }
}
