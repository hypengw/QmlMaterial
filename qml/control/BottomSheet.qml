pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.Popup2 {
    id: control

    property alias mdState: m_state
    property int sheetType: MD.Enum.BottomSheetModal
    property bool showDragHandle: true
    property bool dismissOnDragDown: sheetType === MD.Enum.BottomSheetModal
    property real lowHeight: -1
    property real maxSheetWidth: 640
    property real wideSideMargin: 56
    property real topMargin: 72
    property int animationDuration: 250
    readonly property real sheetWidth: _sheetWidth
    default property alias content: m_content.data

    property real _slideOffset: _visibleHeight
    property real _scrimOpacity: 0
    property bool _dragDismissPending: false

    readonly property real _parentWidth: parent ? parent.width : width
    readonly property real _parentHeight: parent ? parent.height : height
    readonly property real _windowWidth: Window.window ? Window.window.width : _parentWidth
    readonly property real _windowHeight: Window.window ? Window.window.height : _parentHeight
    readonly property real _overlayWidth: fullScreen ? (overlayWidth > 0 ? overlayWidth : _windowWidth) : _parentWidth
    readonly property real _overlayHeight: fullScreen ? (overlayHeight > 0 ? overlayHeight : _windowHeight) : _parentHeight
    readonly property bool _useMaxWidth: _overlayWidth >= maxSheetWidth + wideSideMargin * 2
    readonly property real _sheetWidth: _useMaxWidth ? maxSheetWidth : _overlayWidth
    readonly property real _availableHeight: Math.max(0, _overlayHeight - topMargin)
    readonly property real _handleTouchHeight: showDragHandle ? 48 : 0
    readonly property real _contentHeight: Math.max(0, m_content.childrenRect.height)
    readonly property real _sheetHeight: Math.max(_handleTouchHeight, _contentHeight + _handleTouchHeight)
    readonly property real _automaticLowHeight: Math.min(_sheetHeight, _availableHeight)
    readonly property real _requestedLowHeight: lowHeight > 0 ? lowHeight : _automaticLowHeight
    readonly property real _visibleHeight: Math.min(_sheetHeight, _availableHeight, Math.max(_handleTouchHeight, _requestedLowHeight))
    readonly property real _scrollRange: Math.max(0, _sheetHeight - _visibleHeight)

    x: 0
    y: 0
    width: _overlayWidth
    height: _overlayHeight
    z: 1000
    modal: sheetType === MD.Enum.BottomSheetModal
    dim: modal
    fullScreen: modal
    deferredCompletion: true
    popupItem: m_panel
    closePolicy: modal ? MD.Popup2.CloseOnEscape | MD.Popup2.CloseOnPressOutside : MD.Popup2.NoAutoClose

    onAboutToShow: _startEnter()
    onAboutToHide: _startExit()

    function _startEnter() {
        m_sheet_motion.stop();
        m_sheet_flickable.cancelFlick();
        m_sheet_flickable.contentY = 0;
        _slideOffset = _visibleHeight;
        _scrimOpacity = 0;
        _animateTo(0, 1);
    }

    function _startExit() {
        m_sheet_motion.stop();
        m_sheet_flickable.cancelFlick();
        _slideOffset = -m_sheet_flickable.contentY;
        m_sheet_flickable.contentY = 0;
        _animateTo(_visibleHeight, 0);
    }

    function _animateTo(slideOffset, scrimOpacity) {
        m_slide_animation.to = slideOffset;
        m_scrim_animation.to = scrimOpacity;
        if (Math.abs(_slideOffset - slideOffset) < 0.5 && Math.abs(_scrimOpacity - scrimOpacity) < 0.01) {
            _slideOffset = slideOffset;
            _scrimOpacity = scrimOpacity;
            _finishMotion();
            return;
        }
        m_sheet_motion.start();
    }

    function _finishMotion() {
        if (control.entering)
            control.completeEnter();
        else if (control.closing)
            control.completeExit();
    }

    function _finishDrag() {
        if (!dismissOnDragDown || !control.opened || control.closing || m_sheet_flickable.contentY >= -0.5)
            return;
        _dragDismissPending = true;
        Qt.callLater(control._closeAfterDrag);
    }

    function _closeAfterDrag() {
        if (!_dragDismissPending)
            return;
        _dragDismissPending = false;
        if (control.opened && !control.closing)
            control.close();
    }

    Rectangle {
        id: m_scrim
        anchors.fill: parent
        color: control.dim && control.modal ? MD.Util.transparent(control.mdState.ctx.color.scrim, 0.32) : "transparent"
        opacity: control._scrimOpacity
    }

    MD.Flickable2 {
        id: m_sheet_flickable
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: height + control._scrollRange
        topMargin: control.dismissOnDragDown ? control._visibleHeight : 0
        flickableDirection: MD.Flickable2.VerticalFlick
        synchronousDrag: true
        inputMaskMode: MD.Flickable2.CustomItem
        interactionItem: m_panel
        interactive: control.opened && !control.closing

        onDragStarted: m_sheet_motion.stop()
        onDragEnded: control._finishDrag()

        MD.ElevationRectangle {
            id: m_panel
            x: (m_sheet_flickable.width - width) / 2
            y: m_sheet_flickable.height - control._visibleHeight + control._slideOffset
            width: control._sheetWidth
            height: control._sheetHeight
            corners: control.mdState.corners
            color: control.mdState.backgroundColor
            elevation: control.mdState.elevation
            clip: true

            Item {
                id: m_handle_target
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: control._handleTouchHeight
                visible: control.showDragHandle
                enabled: control.showDragHandle

                Item {
                    width: 72
                    height: 40
                    anchors.centerIn: parent

                    MD.FocusIndicator {
                        anchors.fill: parent
                        active: control.visualFocus
                        corners: MD.Util.corners(MD.Token.shape.corner.large)
                    }
                }

                MD.DragHandle {
                    anchors.centerIn: parent
                    controlWidth: parent.width
                    orientation: Qt.Vertical
                    pressed: m_sheet_flickable.dragging
                    hovered: m_handle_hover.hovered
                    visualFocus: control.visualFocus
                }

                HoverHandler {
                    id: m_handle_hover
                    cursorShape: m_sheet_flickable.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                }
            }

            Item {
                id: m_content
                x: 0
                y: control._handleTouchHeight
                width: parent.width
                height: Math.max(0, parent.height - y)
            }
        }
    }

    MD.StateBottomSheet {
        id: m_state
        item: control
        type: control.sheetType
    }

    ParallelAnimation {
        id: m_sheet_motion

        onFinished: control._finishMotion()

        NumberAnimation {
            id: m_slide_animation
            target: control
            property: "_slideOffset"
            duration: control.animationDuration
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            id: m_scrim_animation
            target: control
            property: "_scrimOpacity"
            duration: control.animationDuration
            easing.type: Easing.OutCubic
        }
    }
}
