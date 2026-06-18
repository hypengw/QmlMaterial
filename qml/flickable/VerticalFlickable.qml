import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Flickable {
    id: root

    readonly property real viewportHeight: height - topMargin - bottomMargin

    readonly property real implicitContentHeight: {
        if (!contentItem) {
            return 0;
        }
        let maxH = 0;
        for (let i = 0; i < contentItem.children.length; ++i) {
            const child = contentItem.children[i];
            maxH = Math.max(maxH, child.y + Math.max(child.implicitHeight, child.height));
        }
        return maxH;
    }

    contentHeight: implicitContentHeight
    contentWidth: width - rightMargin - leftMargin

    T.ScrollBar.vertical: MD.ScrollBar {
        policy: T.ScrollBar.AsNeeded
    }
}
