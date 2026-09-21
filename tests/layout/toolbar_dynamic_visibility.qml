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
    property alias maxShowActionNum: actionBarHost.maxShowActionNum

    Action {
        id: linkAction
        visible: root.linkVisible
    }
    Action {
        id: infoAction
    }
    Action {
        id: closeAction
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
