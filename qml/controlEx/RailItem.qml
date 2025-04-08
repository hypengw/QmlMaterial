import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property alias mdState: m_sh.state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size
    property int lineHeight: MD.Token.typescale.label_large.line_height

    contentItem: ColumnLayout {
        spacing: 4
        MD.Control {
            Layout.alignment: Qt.AlignHCenter
            hoverEnabled: false
            focusPolicy: Qt.NoFocus
            leftInset: 12
            rightInset: 12

            contentItem: MD.Icon {
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.supportTextColor
                fill: control.checked
            }

            background: Item {
                implicitWidth: 56
                implicitHeight: 32
                MD.ElevationRectangle {
                    anchors.centerIn: parent
                    height: parent.height
                    width: 56

                    NumberAnimation on width {
                        alwaysRunToEnd: true
                        from: 48
                        to: 56
                        duration: 200
                        running: control.checked
                    }

                    radius: height / 2
                    color: control.mdState.backgroundColor
                    elevation: control.mdState.elevation
                }
                MD.Ripple2 {
                    readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                    anchors.fill: parent
                    radius: height / 2
                    pressX: p.x
                    pressY: p.y
                    pressed: control.pressed
                    stateOpacity: control.mdState.stateLayerOpacity
                    color: control.mdState.stateLayerColor
                }
            }
        }

        MD.Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            font.capitalization: Font.Capitalize
            typescale: MD.Token.typescale.label_medium
            text: control.text
            prominent: control.checked
        }
    }

    background: Item {}
    MD.StateHolder {
        id: m_sh
        state: MD.StateRailItem {
            item: control
        }
    }
}
