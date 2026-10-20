pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.SideSheetBase {
    id: control
    property bool animationsEnabled: true
    property color color: MD.Token.color.surface
    property int elevation: MD.Token.elevation.level0
    property int radius: detached ? MD.Token.shape.corner.large : 0
    property int __revision: 0

    sheetItem.contentItem.clip: true
    sheetItem.background: MD.ElevationRectangle {
        color: control.color
        elevation: control.elevation
        corners: control.detached ? MD.Util.corners(control.radius) : control.effectiveEdge === Qt.LeftEdge ? MD.Util.corners(0, control.radius, 0, control.radius) : MD.Util.corners(control.radius, 0, control.radius, 0)
    }

    onTransitionRequested: (revision, target, animate) => {
        m_animation.stop();
        __revision = revision;
        if (animate && animationsEnabled && position !== target) {
            m_animation.from = position;
            m_animation.to = target;
            m_animation.start();
        } else {
            position = target;
            completeTransition(revision);
        }
    }
    onStateChanged: {
        if (dragging)
            m_animation.stop();
    }
    onAnimationsEnabledChanged: {
        if (!animationsEnabled && m_animation.running) {
            m_animation.stop();
            completeTransition(__revision);
        }
    }
    NumberAnimation {
        id: m_animation
        target: control
        property: "position"
        duration: MD.Token.duration.medium2
        easing: MD.Token.easing.standard
        onFinished: control.completeTransition(control.__revision)
    }
}
