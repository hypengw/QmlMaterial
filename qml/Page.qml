import QtQuick
import QtQuick.Controls as QC
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    property bool canBack: false
    property T.Action leadingAction: pageContext?.barAction ?? null
    property QtObject pageContext: {
        StackView.view;
    }
    header: MD.AppBar {
        title: control.title
        type: MD.Enum.AppBarSmall
        leadingAction: control.leadingAction
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    property int elevation: MD.Token.elevation.level0
    property color backgroundColor: MD.MatProp.color.background

    font.capitalization: Font.Capitalize

    background: Rectangle {
        color: control.backgroundColor

        layer.enabled: control.enabled && control.elevation > 0
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.elevation
        }
    }
}
