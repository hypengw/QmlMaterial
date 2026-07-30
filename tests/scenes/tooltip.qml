import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 600
    height: 300
    color: MD.MProp.color.surface

    MD.Button {
        id: m_attached_button
        x: 80
        y: 96
        text: "Attached"

        MD.ToolTip.visible: true
        MD.ToolTip.delay: 0
        MD.ToolTip.text: "Attached tooltip"
    }

    MD.Button {
        id: m_plain_button
        x: 250
        y: 96
        text: "Plain popup"

        MD.PlainToolTip {
            parent: m_plain_button
            visible: true
            delay: 0
            text: "Plain tooltip"
        }
    }

    MD.RichToolTip {
        parent: m_plain_button
        x: (m_plain_button.width - width) / 2
        y: m_plain_button.height + 8
        visible: true
        subhead: "Rich tooltip"
        text: "Rich tooltips bring attention to a UI element."
    }
}
