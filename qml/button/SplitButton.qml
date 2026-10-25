import QtQuick
import Qcm.Material as MD

MD.SplitButtonBase {
    id: control
    property bool animationsEnabled: true
    property bool expanded: false
    property MD.springSpec shapeSpringSpec: MD.Token.motion.spring.standard.defaultEffects
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
    }

    property alias action: m_button.action
    property alias text: m_button.text
    property alias icon: m_button.icon
    leadingControl: m_button
    trailingControl: m_indicator

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    hoverEnabled: control.enabled

    spacing: mdState.betweenSpace

    contentItem: Item {
        implicitHeight: Math.max(m_button.implicitHeight, m_indicator.implicitHeight)
        implicitWidth: m_button.implicitWidth + m_indicator.implicitWidth + control.spacing

        MD.ActionButton {
            id: m_button
            x: control.mirrored ? m_indicator.width + control.spacing : 0
            anchors.verticalCenter: parent.verticalCenter
            mdState: control.mdState
            animationsEnabled: control.animationsEnabled
            shapeSpringSpec: control.shapeSpringSpec
            width: parent.width - m_indicator.width - control.spacing
        }

        MD.SplitButtonIndicator {
            id: m_indicator
            expanded: control.expanded
            mdState: control.indicatorMdState
            anchors.verticalCenter: parent.verticalCenter
            x: control.mirrored ? 0 : parent.width - width
            animationsEnabled: control.animationsEnabled
            shapeSpringSpec: control.shapeSpringSpec
            enabled: control.enabled
        }
    }
}
