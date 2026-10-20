pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    property bool bottomToTop: false

    model: m_snake

    MD.SnakeModel {
        id: m_snake
    }

    function show(text, duration = 4000, flag = MD.Enum.TFCloseable, action = null, maximumWidth = 0, actionOnNewLine = false) {
        m_snake.show(text, duration, flag, action, maximumWidth, actionOnNewLine);
    }

    z: Infinity
    spacing: 5
    verticalLayoutDirection: bottomToTop ? ListView.BottomToTop : ListView.TopToBottom
    interactive: false

    remove: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                to: 0
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.linear
            }
        }
    }

    add: Transition {
        id: addTrans
        ParallelAnimation {
            NumberAnimation {
                properties: "height"
                easing: MD.Token.easing.standard
                from: 0
                duration: MD.Token.duration.short4
            }
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.linear
            }
        }
    }

    header: Item {
        implicitHeight: 32
    }

    delegate: Item {
        id: dg_bar
        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        width: Math.min(Math.max(implicitWidth, 200), model.maximumWidth > 0 ? Math.min(model.maximumWidth, ListView.view.width) : ListView.view.width)
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight
        height: implicitHeight
        clip: height < implicitHeight
        required property var model

        MD.SnakeBar {
            x: 0
            y: 0
            width: parent.width
            height: implicitHeight
            text: dg_bar.model.text
            snakeModel: m_snake
            snakeId: dg_bar.model.sid
            presentationId: dg_bar.model.presentationId
            showClose: !!(dg_bar.model.flag & MD.Enum.TFCloseable)
            action: dg_bar.model.action
            actionOnNewLine: dg_bar.model.actionOnNewLine
        }
    }
}
