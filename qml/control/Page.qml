import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    property bool canBack: false
    property MD.Action leadingAction: MD.MatProp.page.leadingAction
    property list<MD.Action> actions

    property alias showHeader: control.header.visible
    property alias showBackground: control.background.visible

    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MatProp.color.background
    property int radius: MD.MatProp.page.radius
    property int backgroundRadius: MD.MatProp.page.backgroundRadius
    property int headerBackgroundOpacity: MD.MatProp.page.headerBackgroundOpacity
    property bool scrolling: false

    header: MD.AppBar {
        title: control.title
        leadingAction: control.leadingAction
        type: control.MD.MatProp.page.headerType
        visible: control.MD.MatProp.page.showHeader
        radius: control.backgroundRadius
        mdState.backgroundOpacity: control.headerBackgroundOpacity
        scrolling: control.scrolling
        actions: control.actions
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    // bottomPadding: header.visible ? radius : 0
    font.capitalization: Font.Capitalize

    background: MD.Rectangle {
        color: control.backgroundColor
        radius: control.backgroundRadius
        visible: control.MD.MatProp.page.showBackground
    }
}
