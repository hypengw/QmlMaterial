import QtQuick
import QtQuick.Controls as QC
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    property bool canBack: false
    property QtObject pageContext: {
        StackView.view;
    }
    readonly property bool _canBack: canBack || (pageContext?.canBack ?? false)

    header: MD.AppBar {
        visible: MD.Token.window_class.compact.contains(Window.window?.width)
        title: control.title
        leadingAction: pageContext?.barAction ?? null //control._canBack ? m_back_action : (Window.window?.barAction ?? null)

        //QC.Action {
        //    id: m_back_action
        //    icon.name: MD.Token.icon.arrow_back
        //    onTriggered: {
        //        if (control.canBack) {
        //            control.back();
        //        } else if (control.pageContext?.canBack) {
        //            control.pageContext.back();
        //        }
        //    }
        //}
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
