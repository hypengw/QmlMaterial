import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Pane {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 12
    property real radius
    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MatProp.color.background

    background: Rectangle {
        color: control.backgroundColor
        radius: control.radius

        layer.enabled: control.enabled && control.elevation > 0
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.elevation
        }
    }
}