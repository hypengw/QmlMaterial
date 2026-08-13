pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.TextFieldEmbed {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property string leadingIcon
    property string trailingIcon
    property MD.StateTextField mdState: MD.StateTextField {
        item: control
    }
    Binding {
        control.mdState.type: control.type
    }

    font.capitalization: Font.MixedCase
    typescale: control.mdState.typescale
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, Math.max(contentWidth, m_placeholder.implicitWidth) + leftPadding + rightPadding)
    implicitHeight: mdState.containerHeight

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? m_placeholder.largestHeight / 2 : 0
    bottomInset: 0

    leftPadding: leading.visible
        ? mdState.horizontalPadding + mdState.spacing + leading.implicitWidth
        : mdState.horizontalPadding
    rightPadding: trailing.visible
        ? mdState.horizontalPadding + mdState.spacing + trailing.implicitWidth
        : mdState.horizontalPadding

    bottomPadding: {
        if (mdState.type === MD.Enum.TextFieldFilled)
            return mdState.verticalPadding / 2;
        else
            return mdState.verticalPadding;
    }
    topPadding: {
        if (mdState.type === MD.Enum.TextFieldFilled) {
            return mdState.containerHeight - contentHeight - bottomPadding;
        } else {
            return mdState.verticalPadding;
        }
    }

    MD.FloatingPlaceholderText {
        id: m_placeholder
        x: control.leftPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.placeholderText
        font: control.font
        color: control.mdState.placeholderColor
        opacity: control.mdState.placeholderOpacity
        elide: Text.ElideRight
        renderType: control.renderType

        controlFocus: control.activeFocus
        controlHeight: control.height
        verticalPadding: control.mdState.verticalPadding / 2

        filled: control.type === MD.Enum.TextFieldFilled
        controlHasText: control.length > 0
        cutoutColor: control.type === MD.Enum.TextFieldFilled
                     ? control.mdState.backgroundColor
                     : "transparent"
        //controlImplicitBackgroundHeight: control.implicitBackgroundHeight
    }

    property Item leading: MD.Icon {
        anchors.left: parent?.left
        anchors.verticalCenter: parent?.verticalCenter
        anchors.leftMargin: control.mdState.horizontalPadding
        name: control.leadingIcon
        visible: name
        size: control.mdState.iconSize
    }

    property Item trailing: MD.Icon {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: control.mdState.horizontalPadding
        visible: name
        name: control.trailingIcon
        size: control.mdState.iconSize
    }

    Item {
        anchors.fill: parent
        data: [m_placeholder, control.leading, control.trailing]
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: control.mdState.containerHeight

        MD.Loader {
            anchors.fill: parent
            sourceComponent: control.type == MD.Enum.TextFieldFilled ? m_filled_comp : m_outline_comp
        }
        Component {
            id: m_filled_comp
            MD.FilledTextFieldShape {
                color: control.mdState.backgroundColor
                radius: MD.Token.shape.corner.extra_small
                bottomLineColor: control.mdState.indicatorColor
                bottomLineWidth: control.mdState.indicatorHeight
            }
        }
        Component {
            id: m_outline_comp
            MD.OutlineTextFieldShape {
                borderColor: control.mdState.outlineColor
                radius: MD.Token.shape.corner.extra_small
                floatWidth: m_placeholder.implicitWidth * m_placeholder.targetScale + 8
                floatX: m_placeholder.x - 4
                open: m_placeholder.text.length > 0 && m_placeholder.floated
            }
        }
    }
    color: control.mdState.textColor
    placeholderTextColor: control.mdState.placeholderColor
}
