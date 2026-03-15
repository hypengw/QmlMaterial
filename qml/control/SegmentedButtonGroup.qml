import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Container {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    spacing: -1

    MD.ButtonGroup {
        id: m_group
        exclusive: true
    }

    contentItem: Row {
        spacing: control.spacing
        Repeater {
            id: m_repeater
            model: control.contentModel
            onCountChanged: control.updatePositions()
        }
    }

    function updatePositions() {
        for (let i = 0; i < m_repeater.count; i++) {
            let item = m_repeater.itemAt(i);
            if (!item) continue;

            if (item.hasOwnProperty("position")) {
                if (m_repeater.count === 1) {
                    item.position = MD.Enum.PosSingle;
                } else if (i === 0) {
                    item.position = MD.Enum.PosFirst;
                } else if (i === m_repeater.count - 1) {
                    item.position = MD.Enum.PosLast;
                } else {
                    item.position = MD.Enum.PosMiddle;
                }
            }
            if (item instanceof T.Button || item instanceof T.AbstractButton) {
                item.ButtonGroup.group = m_group;
            }
        }
    }

    Component.onCompleted: updatePositions()

    property alias exclusive: m_group.exclusive
    property alias group: m_group
}
