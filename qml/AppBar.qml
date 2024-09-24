import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QC
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control
    property alias mdState: m_sh.state
    property int type: MD.Enum.AppBarCenterAligned

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    leftPadding: 16 - m_leading.leftInset
    rightPadding: leftPadding

    property list<QC.Action> actions
    property alias leadingAction: m_leading.action
    property alias title: m_title.text
    property bool onSroll: false

    contentItem: Item {
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight

        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width

            MD.IconButton {
                id: m_leading
                visible: action
                action: Window.window?.barAction ?? null
            }

            MD.Text {
                id: m_title
                Layout.fillWidth: true
                horizontalAlignment: control.type == MD.Enum.AppBarCenterAligned ? Text.AlignHCenter : Text.AlignLeft
                font.capitalization: Font.Capitalize
                typescale: MD.Token.typescale.title_large
            }

            Item {
                visible: control.actions.length === 0
                implicitWidth: m_leading.implicitWidth
            }

            RowLayout {
                id: m_trailing
                Repeater {
                    model: control.actions
                    MD.IconButton {
                        required property QC.Action modelData
                        action: modelData
                    }
                }
            }
        }
    }

    background: Rectangle {
        implicitHeight: 64

        radius: 0
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        layer.enabled: control.enabled
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.mdState.elevation
        }
    }

    MD.StateHolder {
        id: m_sh
        state: MD.StateAppBar {
            item: control
        }
    }
}
