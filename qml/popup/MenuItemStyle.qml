pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.StateMenuItem {
    readonly property var control: item
    property Item arrow: MD.Icon {
        parent: content_layout

        visible: control.subMenu
        size: 24
        name: MD.Token.icon.arrow_right
    }

    readonly property Item contentItem: Lite.Row {
        id: content_layout
        opacity: control.mdState.contentOpacity
        alignment: Qt.AlignVCenter
        spacing: control.spacing

        Item {
            implicitWidth: Math.max(m_leading_icon.implicitWidth, m_leading_loader.implicitWidth)
            implicitHeight: Math.max(m_leading_icon.implicitHeight, m_leading_loader.implicitHeight)
            visible: !control.icon.empty || m_leading_loader.active

            MD.IconView {
                id: m_leading_icon
                anchors.centerIn: parent
                visible: !icon.empty && !m_leading_loader.active
                icon: control.icon
            }

            MD.Loader {
                id: m_leading_loader
                anchors.centerIn: parent
                active: control.busy !== MD.Enum.Idle
                visible: active
                sourceComponent: m_loading_comp
            }
        }

        MD.Label {
            text: control.text
            color: control.mdState.textColor
            useTypescale: false
            font: control.font
            lineHeight: control.typescale.line_height
            wrapMode: Text.NoWrap
            Lite.Layout.fillWidth: true
        }

        Component {
            id: m_loading_comp
            MD.CircularIndicator {
                anchors.centerIn: parent
                indeterminate: control.busy !== MD.Enum.Progress
                running: control.busy !== MD.Enum.Progress
                value: control.progress
                strokeWidth: 2
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
            }
        }
    }

    readonly property Item background: Rectangle {
        implicitWidth: 200
        implicitHeight: 48
        color: control.mdState.backgroundColor

        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            interactionSource: control.interactionSource
            indication: control.mdState.indication
            color: control.mdState.stateLayerColor
        }
    }
}
