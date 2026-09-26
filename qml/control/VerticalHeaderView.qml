pragma ComponentBehavior: Bound

import QtQuick
import Qcm.Material as MD

MD.VerticalHeaderViewBase {
    id: control

    implicitWidth: Math.max(1, contentWidth)
    implicitHeight: syncView ? syncView.height : 0

    delegate: MD.VerticalHeaderViewDelegate {}
}
