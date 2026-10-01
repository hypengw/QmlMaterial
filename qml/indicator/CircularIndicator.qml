pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @brief Material Design 3 circular progress indicator
 * @ingroup component
 */
MD.CircularIndicatorBase {
    id: control
    color: control.MD.MProp.color.primary
    inactiveColor: "transparent"
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: wavy ? 0 : strokeWidth
    clip: false
    contentItem: Item {
        implicitWidth: control.preferredSize.width
        implicitHeight: control.preferredSize.height
        MD.ProgressIndicatorShape {
            anchors.fill: parent
            source: control
        }
    }
    background: Item {}
}
