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
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? m_placeholder.largestHeight / 2 : 0
    bottomInset: 0

    leftPadding: leading.visible ? 16 + 12 + leading.implicitWidth : 16
    rightPadding: trailing.visible ? 16 + 12 + trailing.implicitWidth : 16

    bottomPadding: {
        if (mdState.type === MD.Enum.TextFieldFilled)
            return mdState.bottomPadding / 2;
        else
            return mdState.bottomPadding;
    }
    topPadding: {
        if (mdState.type === MD.Enum.TextFieldFilled) {
            const ch = cursorRectangle.height;
            const ph = m_placeholder.implicitHeight;
            return ph + mdState.topPadding / 2;
        } else {
            return mdState.topPadding;
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
        verticalPadding: 8

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
        anchors.leftMargin: 12
        name: control.leadingIcon
        visible: name
        size: 24
    }

    property Item trailing: MD.Icon {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 12
        visible: name
        name: control.trailingIcon
        size: 24
    }

    Item {
        anchors.fill: parent
        data: [m_placeholder, control.leading, control.trailing]
    }

    background: Item {
        implicitWidth: 64

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
                open: control.activeFocus || control.text.length > 0
            }
        }
    }
    color: control.mdState.textColor
    placeholderTextColor: control.mdState.placeholderColor
}
