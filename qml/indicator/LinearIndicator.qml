pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @brief Material Design 3 linear progress indicator
 * @ingroup component
 */
MD.LinearIndicatorBase {
    id: control
    color: control.MD.MProp.color.primary
    trackColor: control.MD.MProp.color.secondary_container
    stopIndicatorColor: control.color
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: 0
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
