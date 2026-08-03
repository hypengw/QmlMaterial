import QtQuick
import QtQuick.Templates as T
import QtQuick.Window

import Qcm.Material as MD

T.Popup {
    id: control

    property alias mdState: item_state
    property int radius: MD.Token.shape.corner.large
    property bool readyForOpen: true
    readonly property bool openPending: m_open_state.pending

    signal openRejected(string error)

    function requestOpen() {
        if (m_open_state.rejected) {
            if (m_open_state.replayRejection) {
                m_open_state.replayRejection = false;
                control.openRejected(m_open_state.error);
            }
            return;
        }
        if (control.visible || m_open_state.pending)
            return;

        if (control.readyForOpen) {
            control.open();
        } else {
            m_open_state.pending = true;
        }
    }

    function rejectOpen(error) {
        if (m_open_state.rejected || control.visible)
            return;

        const requested = m_open_state.pending;
        m_open_state.pending = false;
        m_open_state.rejected = true;
        m_open_state.replayRejection = !requested;
        m_open_state.error = String(error ?? "");
        control.openRejected(m_open_state.error);
    }

    onReadyForOpenChanged: {
        if (readyForOpen && m_open_state.pending) {
            m_open_state.pending = false;
            control.open();
        }
    }
    onClosed: {
        m_open_state.pending = false;
        m_open_state.rejected = false;
        m_open_state.replayRejection = false;
        m_open_state.error = "";
    }

    QtObject {
        id: m_open_state
        property bool pending: false
        property bool rejected: false
        property bool replayRejection: false
        property string error: ""
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    margins: 0
    verticalPadding: 0
    horizontalPadding: 1
    property MD.typescale typescale: MD.Token.typescale.body_medium

    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    MD.MProp.size: MD.ThemeSize {
        width: control.parent.width
        duration: 0
    }

    enter: Transition {
        // grow_fade_in
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    exit: Transition {
        // shrink_fade_out
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 48
        radius: control.radius
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    MD.MState {
        id: item_state

        elevation: MD.Token.elevation.level2
        textColor: MD.Token.color.on_surface
        backgroundColor: MD.Token.color.surface_container
        supportTextColor: MD.Token.color.on_surface_variant
    }
}
