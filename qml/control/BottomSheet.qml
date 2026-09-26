pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.PopupBase {
    id: control

    property alias mdState: m_state
    property int sheetType: MD.Enum.BottomSheetModal
    property bool showDragHandle: true
    property bool nestedScrollEnabled: false
    property bool dismissOnDragDown: sheetType === MD.Enum.BottomSheetModal
    property real dragDismissThreshold: _collapsedHeight * 0.25
    property real lowHeight: -1
    property real collapsedHeight: -1
    property real maxSheetWidth: 640
    property real wideSideMargin: 56
    topMargin: 72
    property int animationDuration: 250
    readonly property real sheetWidth: _sheetWidth
    property real preferredContentHeight: -1
    readonly property real contentViewportWidth: _sheetWidth
    readonly property real contentViewportHeight: Math.max(0, _visibleHeight - _handleTouchHeight)
    default property alias content: m_content.data

    property real _slideOffset: _visibleHeight
    property real _scrimOpacity: 0
    property bool _dragDismissPending: false
    property real _dragReleasePosition: 0
    property real _dragReleaseVelocity: 0

    readonly property real _parentWidth: parent ? parent.width : width
    readonly property real _parentHeight: parent ? parent.height : height
    readonly property real _windowWidth: overlayWidth > 0 ? overlayWidth : _parentWidth
    readonly property real _windowHeight: overlayHeight > 0 ? overlayHeight : _parentHeight
    readonly property real _overlayWidth: modal ? _windowWidth : _parentWidth
    readonly property real _overlayHeight: modal ? _windowHeight : _parentHeight
    readonly property bool _useMaxWidth: _overlayWidth >= maxSheetWidth + wideSideMargin * 2
    readonly property real _sheetWidth: _useMaxWidth ? maxSheetWidth : _overlayWidth
    readonly property real _availableHeight: Math.max(0, _overlayHeight - topMargin)
    readonly property real _handleTouchHeight: showDragHandle ? 48 : 0
    readonly property real _naturalContentHeight: Math.max(0, m_content.childrenRect.height)
    readonly property real _contentHeight: preferredContentHeight >= 0 ? Math.min(preferredContentHeight, Math.max(0, _availableHeight - _handleTouchHeight)) : _naturalContentHeight
    readonly property real _sheetHeight: Math.max(_handleTouchHeight, _contentHeight + _handleTouchHeight)
    readonly property real _automaticLowHeight: Math.min(_sheetHeight, _availableHeight)
    readonly property real _requestedLowHeight: lowHeight > 0 ? lowHeight : _automaticLowHeight
    readonly property real _visibleHeight: Math.min(_sheetHeight, _availableHeight, Math.max(_handleTouchHeight, _requestedLowHeight))
    readonly property real _collapsedHeight: collapsedHeight > 0 ? Math.min(_visibleHeight, collapsedHeight) : _visibleHeight
    readonly property real _collapseDistance: Math.max(0, _visibleHeight - _collapsedHeight)
    readonly property real _dismissDistance: dismissOnDragDown ? Math.max(1, _collapsedHeight) : 0
    readonly property real _dragDownRange: _collapseDistance + _dismissDistance
    readonly property real _scrollRange: Math.max(0, _sheetHeight - _visibleHeight)

    x: 0
    y: 0
    width: _overlayWidth
    height: _overlayHeight
    modal: sheetType === MD.Enum.BottomSheetModal
    focus: modal
    dim: modal
    positioningItem: modal ? overlayItem : null
    collisionPolicy: MD.PopupBase.Unrestricted
    deferredCompletion: true
    popupItem: m_panel
    closePolicy: modal ? MD.PopupBase.CloseOnEscape | MD.PopupBase.CloseOnPressOutside : MD.PopupBase.NoAutoClose

    onAboutToShow: _startEnter()
    onAboutToHide: _startExit()
    onDragDismissThresholdChanged: {
        if (m_sheet_flickable.dragging)
            _updateDragScrim();
    }
    onDismissOnDragDownChanged: {
        if (m_sheet_flickable.dragging)
            _updateDragScrim();
    }
    onClosed: {
        m_drag_return.stop();
        m_drag_scrim.stop();
        m_sheet_motion.stop();
        m_sheet_flickable.cancelFlick();
        _dragDismissPending = false;
        _scrimOpacity = 0;
        _slideOffset = _visibleHeight;
    }

    function _startEnter() {
        m_drag_return.stop();
        m_drag_scrim.stop();
        m_sheet_motion.stop();
        m_sheet_flickable.cancelFlick();
        m_sheet_flickable.contentY = 0;
        _slideOffset = _visibleHeight;
        _scrimOpacity = 0;
        _animateTo(0, 1);
    }

    function _startExit() {
        m_drag_return.stop();
        m_drag_scrim.stop();
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

    function _willDismissAt(position, velocity) {
        // Positive content velocity moves the sheet back up; reversal wins over distance.
        const projectedPosition = position + velocity * 0.1;
        return dismissOnDragDown && velocity <= 0 && position < -_collapseDistance - 0.5 && -projectedPosition - _collapseDistance >= Math.max(0, dragDismissThreshold);
    }

    function _updateDragScrim() {
        if (!control.opened || control.closing)
            return;
        const target = _willDismissAt(m_sheet_flickable.contentY, m_sheet_flickable.dragVelocity.y) ? 0 : 1;
        _animateDragScrim(target);
    }

    function _animateDragScrim(target) {
        if (m_drag_scrim.to === target && m_drag_scrim.running || !m_drag_scrim.running && _scrimOpacity === target)
            return;
        m_drag_scrim.stop();
        m_drag_scrim.to = target;
        m_drag_scrim.start();
    }

    function _finishDrag(velocity) {
        _updateDragScrim();
        if (!dismissOnDragDown || !control.opened || control.closing || m_sheet_flickable.contentY >= -control._collapseDistance - 0.5)
            return;
        _dragDismissPending = true;
        _dragReleasePosition = m_sheet_flickable.contentY;
        _dragReleaseVelocity = velocity;
        Qt.callLater(control._settleAfterDrag);
    }

    function _settleAfterDrag() {
        if (!_dragDismissPending)
            return;
        m_sheet_flickable.cancelFlick();
        _dragDismissPending = false;
        if (!control.opened || control.closing || m_sheet_flickable.dragging)
            return;
        m_sheet_flickable.contentY = _dragReleasePosition;
        if (_willDismissAt(_dragReleasePosition, _dragReleaseVelocity)) {
            control.close();
        } else {
            m_drag_return.to = -_collapseDistance;
            m_drag_return.start();
        }
    }

    Rectangle {
        id: m_scrim
        anchors.fill: parent
        color: control.dim && control.modal ? MD.Util.transparent(control.mdState.ctx.color.scrim, 0.32) : "transparent"
        opacity: control._scrimOpacity
    }

    MD.Scrollable {
        id: m_sheet_flickable
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: height + control._scrollRange
        topMargin: control._dragDownRange
        flickableDirection: MD.Scrollable.VerticalFlick
        synchronousDrag: true
        MD.NestedScroll.enabled: control.nestedScrollEnabled
        MD.NestedScroll.wheelEnabled: false
        MD.NestedScroll.restoreOnReverse: true
        onMovementEnded: {
            if (!control._dragDismissPending && !dragging && contentY < -control._collapseDistance)
                contentY = -control._collapseDistance;
        }
        inputMaskMode: MD.Scrollable.CustomItem
        interactionItem: m_panel
        interactive: control.opened && !control.closing

        onDragStarted: {
            control._dragDismissPending = false;
            m_drag_return.stop();
            m_sheet_motion.stop();
            control._updateDragScrim();
        }
        onContentYChanged: {
            if (dragging)
                control._updateDragScrim();
        }
        onDragVelocityChanged: {
            if (dragging)
                control._updateDragScrim();
        }
        onDragReleased: velocity => control._finishDrag(velocity.y)
        onDragEnded: {
            if (!control._dragDismissPending && control.opened && !control.closing)
                control._animateDragScrim(1);
        }

        MD.ElevationRectangle {
            id: m_panel
            x: (m_sheet_flickable.width - width) / 2
            y: m_sheet_flickable.height - control._visibleHeight + control._slideOffset
            width: control._sheetWidth
            height: control._sheetHeight
            corners: control.mdState.corners
            color: control.mdState.backgroundColor
            elevation: control.mdState.elevation

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

    NumberAnimation {
        id: m_drag_return
        target: m_sheet_flickable
        property: "contentY"
        duration: control.animationDuration
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: m_drag_scrim
        target: control
        property: "_scrimOpacity"
        duration: control.animationDuration
        easing.type: Easing.OutCubic
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
