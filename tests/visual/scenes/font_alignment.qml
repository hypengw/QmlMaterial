import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 800
    height: 400
    color: "white"

    Repeater {
        model: [Qt.application.font.family, "serif", "FangSong", "SimSun"]
        Item {
            required property string modelData
            required property int index
            x: 16
            y: index * 100
            width: 768
            height: 100

            Text {
                x: 0
                y: 8
                text: modelData
                font.pixelSize: 14
                color: "#333333"
            }
            Rectangle {
                y: 60
                width: parent.width
                height: 1
                color: "#44cc8888"
            }
            MD.Text {
                x: 0
                y: 60 - height / 2
                text: "\u4e3b\u9898"
                font.family: modelData
                typescale: MD.Token.typescale.label_large
            }
            MD.SegmentedButtonGroup {
                x: 120
                y: 60 - height / 2
                size: MD.Enum.XS
                MD.SegmentedButton {
                    text: "\u6d45\u8272"
                    font.family: modelData
                }
                MD.SegmentedButton {
                    text: "\u6df1\u8272"
                    font.family: modelData
                }
                MD.SegmentedButton {
                    text: "\u7cfb\u7edf"
                    checked: true
                    font.family: modelData
                }
            }
            MD.ComboBox {
                x: 430
                y: 60 - height / 2
                width: 300
                font.family: modelData
                model: ["\u7b80\u4f53\u4e2d\u6587\uff08\u4e2d\u56fd\uff09"]
            }
        }
    }
}
