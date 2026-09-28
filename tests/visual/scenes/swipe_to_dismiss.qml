import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 488
    color: MD.Token.color.surface
    Column {
        x: 24
        y: 24
        spacing: 24
        Repeater {
            model: [0, 0.4, -0.55, 1, -0.4]
            MD.SwipeToDismiss {
                id: row
                required property real modelData
                required property int index
                width: index === 2 ? 260 : 540
                height: 64
                clip: true
                layoutDirection: index === 2 ? Qt.RightToLeft : Qt.LeftToRight
                animationsEnabled: false
                Component.onCompleted: {
                    dismissState.begin(0);
                    dismissState.dragBy(width * modelData);
                }
                background: Rectangle {
                    color: row.dismissState.dismissDirection === MD.SwipeToDismissState.EndToStart ? MD.Token.color.error_container : MD.Token.color.secondary_container
                    MD.Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        x: row.presentedOffset >= 0 ? 16 : parent.width - width - 16
                        name: row.dismissState.dismissDirection === MD.SwipeToDismissState.EndToStart ? 'delete' : 'check'
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    color: MD.Token.color.surface_container_high
                    MD.Text {
                        anchors.fill: parent
                        leftPadding: 16
                        rightPadding: 16
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        text: row.index === 2 ? 'RTL · A long message that keeps its natural size' : 'Message content stays the same width'
                    }
                }
            }
        }
    }
}
