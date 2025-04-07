import QtQuick
import Qcm.Material as MD

MD.TextFieldEmbed {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property string leading_icon
    property string trailing_icon
    property MD.StateTextField mdState: MD.StateTextField {
        item: control
    }

    font.capitalization: Font.MixedCase

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding + (leading.visible ? leading.implicitWidth : 0) + (trailing.visible ? trailing.implicitWidth : 0))
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? placeholder.largestHeight / 2 : 0
    bottomInset: 0

    leftPadding: 16
    rightPadding: 16
    bottomPadding: 0
    topPadding: type === MD.Enum.TextFieldFilled ? 10 + topInset : 0

    // Need to account for the placeholder text when it's sitting on top.
    //topPadding: Material.containerStyle === Material.Filled
    //    ? placeholderText.length > 0 && (activeFocus || length > 0)
    //        ? Material.textFieldVerticalPadding + placeholder.largestHeight
    //        : Material.textFieldVerticalPadding
    //    // Account for any topInset (used to avoid floating placeholder text being clipped),
    //    // otherwise the text will be too close to the background.
    //    : Material.textFieldVerticalPadding + topInset

    MD.FloatingPlaceholderText {
        id: placeholder
        x: control.leftPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.placeholderText
        font: control.font
        color: control.mdState.placeholderColor
        useTypescale: false
        elide: Text.ElideRight
        renderType: control.renderType

        controlFocus: control.activeFocus
        controlHeight: control.height
        verticalPadding: 8

        filled: control.type === MD.Enum.TextFieldFilled
        controlHasText: control.length > 0
        //controlImplicitBackgroundHeight: control.implicitBackgroundHeight
    }

    property Item leading: MD.Icon {
        anchors.left: parent?.left
        anchors.verticalCenter: parent?.verticalCenter
        anchors.leftMargin: 12
        name: control.leading_icon
        visible: name
        size: 24
    }

    property Item trailing: MD.Icon {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 12
        visible: name
        name: control.trailing_icon
        size: 24
    }

    data: [placeholder, leading, trailing]

    background: MD.MaterialTextContainer {
        implicitWidth: 64
        implicitHeight: 56

        filled: control.type === MD.Enum.TextFieldFilled
        fillColor: control.mdState.backgroundColor
        outlineColor: control.mdState.outlineColor
        focusedOutlineColor: control.mdState.outlineColor
        placeholderTextWidth: Math.min(placeholder.width, placeholder.implicitWidth) * 0.8 //placeholder.scale
        controlHasActiveFocus: control.activeFocus
        controlHasText: control.length > 0
        placeholderHasText: placeholder.text.length > 0
        horizontalPadding: 16
    }
    color: control.mdState.textColor
    placeholderTextColor: control.mdState.placeholderColor

    MD.StateHolder {
        state: control.mdState
    }
}
