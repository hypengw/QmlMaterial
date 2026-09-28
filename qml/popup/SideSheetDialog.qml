pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.PopupBase {
    id: control
    default property alias content: m_sheet.contentData
    property int edge: MD.SideSheetBase.End
    property alias detached: m_sheet.detached
    property alias sheetWidth: m_sheet.sheetWidth
    property alias draggable: m_sheet.draggable
    property alias hideFriction: m_sheet.hideFriction
    property alias animationsEnabled: m_sheet.animationsEnabled
    readonly property alias sheetItem: m_sheet.sheetItem
    readonly property alias position: m_sheet.position
    property int __edge: MD.SideSheetBase.End
    property int __layoutDirection: Qt.LeftToRight
    property bool __session: false

    x: 0
    y: 0
    width: overlayWidth
    height: overlayHeight
    positioningItem: overlayItem
    collisionPolicy: MD.PopupBase.Unrestricted
    modal: true
    dim: modal
    focus: true
    deferredCompletion: true
    popupItem: m_sheet.sheetItem
    closePolicy: MD.PopupBase.CloseOnEscape | MD.PopupBase.CloseOnPressOutside
    onAboutToShow: {
        if (!__session) {
            __edge = edge;
            __layoutDirection = mirrored ? Qt.RightToLeft : Qt.LeftToRight;
            __session = true;
        }
        m_sheet.open();
        if (m_sheet.state === MD.SideSheetBase.Expanded)
            completeEnter();
    }
    onAboutToHide: {
        m_sheet.close();
        if (m_sheet.state === MD.SideSheetBase.Hidden)
            completeExit();
    }
    onClosed: __session = false

    contentItem: MD.SideSheet {
        id: m_sheet
        edge: control.__edge
        layoutDirection: control.__layoutDirection
        radius: MD.Token.shape.corner.large
        color: MD.Token.color.surface_container_low
        elevation: MD.Token.elevation.level1
        onExpandedChanged: {
            if (!expanded && control.visible && !control.closing)
                control.close();
        }
        onOpened: {
            if (control.entering)
                control.completeEnter();
        }
        onClosed: {
            if (control.closing)
                control.completeExit();
        }
    }
    MD.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short3
                easing: MD.Token.easing.linear
            }
        }
    }
}
