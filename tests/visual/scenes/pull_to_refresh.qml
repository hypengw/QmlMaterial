import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 280
    color: MD.Token.color.surface
    Row {
        spacing: 16
        x: 16
        y: 16
        Repeater {
            model: [0.65, 1.3, 1]
            MD.PullToRefresh {
                required property real modelData
                required property int index
                width: 192
                height: 248
                animationsEnabled: false
                refreshing: index === 2
                layoutDirection: index === 1 ? Qt.RightToLeft : Qt.LeftToRight
                // Fixed presentation samples; native gesture state is exercised separately.
                Component.onCompleted: __offset = threshold * modelData
                Rectangle {
                    anchors.fill: parent
                    color: MD.Token.color.surface_container_low
                    Column {
                        width: parent.width
                        Repeater {
                            model: 4
                            MD.Text {
                                required property int index
                                width: parent.width
                                height: 56
                                leftPadding: 12
                                text: 'Item ' + (index + 1)
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
