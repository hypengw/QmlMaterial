import QtQuick
import QtQuick.Controls as QC
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    property bool canBack: false
    property T.Action leadingAction: pageContext?.barAction ?? null

    property alias showHeader: control.header.visible
    property alias showBackground: control.background.visible

    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MatProp.color.background
    property int radius: pageContext?.radius ?? 0
    property QtObject pageContext: {
        StackView.view;
    }
    header: MD.AppBar {
        title: control.title
        leadingAction: control.leadingAction
        type: control.pageContext?.barType ?? MD.Enum.AppBarSmall
        visible: control.pageContext?.barVisible ?? true
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    bottomPadding: header.visible ? radius : 0
    font.capitalization: Font.Capitalize

    background: Rectangle {
        color: control.backgroundColor
        radius: control.radius
        visible: false
        // layer.enabled: control.enabled && control.elevation > 0
        // layer.effect: MD.RoundedElevationEffect {
        //     elevation: control.elevation
        // }
    }
}
