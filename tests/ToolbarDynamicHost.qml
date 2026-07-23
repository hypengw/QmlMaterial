import QtQuick
import QtQuick.Layouts
import QmlMaterialTest

Item {
    id: root

    property list<QtObject> actions
    readonly property real targetWidth: Math.ceil(actionLayout.implicitWidth) + 2
    readonly property int hiddenCount: actionLayout.hiddenActions.length

    implicitWidth: targetWidth
    implicitHeight: actionLayout.implicitHeight
    Layout.minimumWidth: targetWidth
    Layout.preferredWidth: targetWidth
    Layout.maximumWidth: targetWidth
    Layout.preferredHeight: actionLayout.implicitHeight

    Component {
        id: actionDelegate
        Item {
            implicitWidth: 40
            implicitHeight: 40
        }
    }

    ToolBarLayout {
        id: actionLayout
        anchors.fill: parent
        actions: root.actions
        fullDelegate: actionDelegate
        iconDelegate: actionDelegate
        separatorDelegate: actionDelegate
        moreButton: actionDelegate
    }
}
