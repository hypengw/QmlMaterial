import QtQuick
import Qcm.Material as MD

// Divider for use inside NavigationDrawer content sections.
// Matches Material 3 spec: 16dp inset, 8dp top margin, 3dp bottom margin.
Item {
    id: root
    implicitWidth: parent ? parent.width : 0
    implicitHeight: m_divider.height + 11 // 8 top + 3 bottom

    Rectangle {
        id: m_divider
        x: 16
        y: 8
        width: root.width - 32
        height: 1
        color: MD.Token.color.outline_variant
    }
}
