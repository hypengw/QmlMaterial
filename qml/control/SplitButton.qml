import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control
    property MD.StateSplitButton mdState: MD.StateSplitButton {
        item: m_button
    }

    property MD.StateSplitButtonIndicator indicatorMdState: MD.StateSplitButtonIndicator {
        item: m_indicator
    }

    // Synchronize indicator state with main state by default
    Binding {
        control.indicatorMdState.type: control.mdState.type
        control.indicatorMdState.size: control.mdState.size
        control.indicatorMdState.isRound: control.mdState.isRound
        control.indicatorMdState.corners: control.mdState.trailingCorners
    }

    property alias action: m_button.action
    property alias text: m_button.text
    property alias icon: m_button.icon
    property alias menu: m_indicator.menu
    signal clicked()

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    spacing: 2

    contentItem: Item {
        implicitHeight: Math.max(m_button.implicitHeight, m_indicator.implicitHeight)
        implicitWidth: m_button.implicitWidth + m_indicator.implicitWidth + control.spacing

        MD.Button {
            id: m_button
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            mdState: control.mdState
            onClicked: control.clicked()
            width: parent.width - m_indicator.width - control.spacing
        }

        MD.SplitButtonIndicator {
            id: m_indicator
            mdState: control.indicatorMdState
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
        }
    }
}
