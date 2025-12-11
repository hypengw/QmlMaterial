import QtQuick
import Qcm.Material as MD

MD.ApplicationWindow {
    width: 640
    height: 420
    visible: true

    // Match the palette setup used in the main example
    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.MProp.backgroundColor: MD.MProp.color.surface_container

    MD.Pane {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Column {
            spacing: 12

            MD.Text {
                text: "Hello QmlMaterial"
                font.pixelSize: MD.Token.typescale.title_large.size
            }

            MD.Text {
                text: "Buttons"
                font.pixelSize: MD.Token.typescale.title_medium.size
            }

            Row {
                spacing: 12
                MD.Button {
                    text: "Primary"
                    type: MD.Enum.BtElevated
                    onClicked: console.log("Primary clicked")
                }
                MD.Button {
                    text: "Outlined"
                    type: MD.Enum.BtOutlined
                }
                MD.Button {
                    text: "Text"
                    type: MD.Enum.BtText
                }
            }

            MD.Text {
                text: "Toggles"
                font.pixelSize: MD.Token.typescale.title_medium.size
            }

            Row {
                spacing: 20
                MD.Switch {
                    id: sw
                    text: "Dark mode"
                }
                MD.CheckBox {
                    checked: true
                    text: "Check me"
                }
            }

            MD.Text {
                text: "Slider"
                font.pixelSize: MD.Token.typescale.title_medium.size
            }

            MD.Slider {
                width: 220
                from: 0
                to: 100
                value: 42
            }
        }
    }
}
