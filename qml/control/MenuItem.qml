pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

T.MenuItem {
    id: control

    property bool selected: false
    readonly property int busy: {
        const a = control.action;
        if (a instanceof MD.Action)
            return (a as MD.Action).busy;
        return MD.Enum.Idle;
    }
    readonly property real progress: {
        const a = control.action;
        if (a instanceof MD.Action)
            return (a as MD.Action).progress;
        return 0;
    }
    property MD.StateMenuItem mdState: MD.StateMenuItem {
        item: control
    }
    Binding {
        control.mdState.selected: control.selected
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 0
    spacing: 16

    icon.width: 24
    icon.height: 24
    icon.color: mdState.textColor

    /*
    indicator: CheckIndicator {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable
        control: control
        checkState: control.checked ? Qt.Checked : Qt.Unchecked
    }
    */

    arrow: MD.Icon {
        parent: content_layout

        visible: control.subMenu
        size: 24
        name: MD.Token.icon.arrow_right
    }

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.MixedCase
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: Lite.Row {
        id: content_layout
        alignment: Qt.AlignVCenter
        spacing: control.spacing

        Item {
            implicitWidth: Math.max(m_leading_icon.implicitWidth, m_leading_loader.implicitWidth)
            implicitHeight: Math.max(m_leading_icon.implicitHeight, m_leading_loader.implicitHeight)
            visible: control.icon.name.length > 0 || m_leading_loader.active

            MD.Icon {
                id: m_leading_icon
                anchors.centerIn: parent
                visible: name.length > 0 && !m_leading_loader.active
                name: control.icon.name
                size: control.icon.width
                color: control.leadingIconColor
                fill: control.checked
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

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 48
        color: control.mdState.backgroundColor

        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

    property color leadingIconColor: control.mdState.leadingIconColor
    property color trailingIconColor: control.mdState.trailingIconColor
}
