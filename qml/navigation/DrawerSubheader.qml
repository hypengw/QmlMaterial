import QtQuick
import Qcm.Material as MD

// Section subheader for use inside NavigationDrawer.
// Matches Material 3 spec: 16dp horizontal padding, 8dp vertical padding.
Item {
    id: root
    property alias text: m_label.text

    implicitWidth: m_label.implicitWidth + 32 // 16 * 2
    implicitHeight: m_label.implicitHeight + 16 + 8 // 8dp vertical padding * 2 + 8dp top margin

    MD.Text {
        id: m_label
        x: 16
        y: 8 + 8 // top margin + top padding
        width: root.width - 32
        typescale: MD.Token.typescale.title_small
        color: MD.Token.color.on_surface_variant
    }
}
