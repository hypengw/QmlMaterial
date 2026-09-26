pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ControlBase {
    id: control

    property list<MD.Action> actions
    property real delegateWidth: 72
    property real delegateHeight: 80
    property real delegateHorizontalPadding: 8
    property real actionSpacing: 8

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8

    contentItem: MD.HorizontalListView {
        id: m_view

        implicitWidth: contentWidth
        implicitHeight: control.delegateHeight
        model: control.actions.length
        spacing: control.actionSpacing
        interactive: contentWidth > width

        delegate: MD.ButtonBase {
            id: m_button

            required property int index
            readonly property MD.Action actionItem: control.actions[index]

            width: Math.max(control.delegateWidth, m_column.implicitWidth + control.delegateHorizontalPadding * 2)
            height: control.delegateHeight
            action: actionItem
            icon.width: 24
            icon.height: 24
            icon.color: contentColor
            icon.fill: checked
            enabled: actionItem ? actionItem.enabled : false
            readonly property color contentColor: {
                if (!m_button.enabled)
                    return MD.Util.transparent(MD.MProp.color.on_surface, 0.38);
                return m_button.checked ? MD.MProp.color.on_primary_container : MD.MProp.color.on_surface_variant;
            }

            contentItem: Item {
                Column {
                    id: m_column

                    anchors.centerIn: parent
                    spacing: 8

                    MD.IconView {
                        anchors.horizontalCenter: parent.horizontalCenter
                        icon: m_button.icon
                    }

                    MD.Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: m_button.text
                        typescale: MD.Token.typescale.label_medium
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.NoWrap
                        maximumLineCount: 1
                        elide: Text.ElideNone
                        color: m_button.enabled ? (m_button.checked ? MD.MProp.color.on_primary_container : MD.MProp.color.on_surface) : m_button.contentColor
                    }
                }
            }

            background: Item {
                Rectangle {
                    anchors.fill: parent
                    radius: MD.Token.shape.corner.extra_large
                    visible: m_button.checked
                    color: MD.MProp.color.primary_container
                }

                MD.Ripple {
                    anchors.fill: parent
                    radius: MD.Token.shape.corner.extra_large
                    pressed: m_button.pressed
                    pressX: m_button.pressX
                    pressY: m_button.pressY
                    stateOpacity: m_button.hovered ? 0.08 : 0
                    color: m_button.checked ? MD.MProp.color.on_primary_container : MD.MProp.color.on_surface
                }

                MD.FocusIndicator {
                    anchors.fill: parent
                    corners: MD.Util.corners(MD.Token.shape.corner.extra_large)
                    active: m_button.visualFocus
                }
            }
        }
    }
}
