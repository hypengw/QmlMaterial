import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 600
    height: 300
    color: MD.MProp.color.surface

    MD.Button {
        id: m_button
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80
        text: "Hover me"

        MD.ToolTip {
            visible: true
            delay: 0
            text: "Plain tooltip"
        }
    }

    MD.RichToolTip {
        parent: m_button
        x: (m_button.width - width) / 2
        y: m_button.height + 8
        visible: true
        subhead: "Rich tooltip"
        text: "Rich tooltips bring attention to a UI element."
    }
}
