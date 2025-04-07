import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

Rectangle {
    id: root
    property bool pressed: false
    property bool hovered: false
    property bool visualFocus: false

    property int orientation: Qt.Vertical
    property int controlHeight: 0
    property int controlWidth: 0

    property alias mdState: m_state

    color: mdState.backgroundColor

    Binding {
        when: root.orientation === Qt.Horizontal
        root.implicitWidth: 24
        root.implicitHeight: root.controlHeight
        m_bar.width: root.mdState.handleWidth
        m_bar.height: root.mdState.handleHeight
    }
    Binding {
        when: root.orientation === Qt.Vertical
        root.implicitWidth: root.controlWidth
        root.implicitHeight: 24
        m_bar.width: root.mdState.handleHeight
        m_bar.height: root.mdState.handleWidth
    }

    Rectangle {
        id: m_bar
        color: root.mdState.textColor
        radius: root.mdState.radius
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.mdState.stateLayerColor
            opacity: root.mdState.stateLayerOpacity
        }
    }

    MD.StateDragHandle {
        id: m_state
        item: root

        Behavior on handleHeight {
            NumberAnimation {
                duration: 100
            }
        }
        Behavior on handleWidth {
            NumberAnimation {
                duration: 100
            }
        }
    }
}
