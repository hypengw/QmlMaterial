pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.TextFieldEmbed {
    id: control
    property bool __labelAnimationsEnabled: false
    Component.onCompleted: __labelAnimationsEnabled = true

    property int type: MD.Enum.TextFieldOutlined
    property string leadingIcon
    property string trailingIcon
    property bool error: !acceptableInput
    property string supportingText
    property string errorText
    property string prefix
    property string suffix
    readonly property bool mirrored: LayoutMirroring.enabled
    readonly property real supportingHeight: support.text.length > 0 ? support.implicitHeight + mdState.sizeTokens.supporting_top_padding : 0
    readonly property real containerHeight: Math.max(0, height - supportingHeight)
    readonly property real __leadingPadding: leading && leading.visible ? mdState.horizontalPadding + mdState.spacing + leading.implicitWidth : mdState.horizontalPadding
    readonly property real __trailingPadding: trailing && trailing.visible ? mdState.horizontalPadding + mdState.spacing + trailing.implicitWidth : mdState.horizontalPadding
    readonly property bool __showPrefix: prefix.length > 0 && (placeholderText.length === 0 || m_placeholder.floated)
    readonly property bool __showSuffix: suffix.length > 0 && (placeholderText.length === 0 || m_placeholder.floated)
    readonly property real __affixAvailableWidth: Math.max(0, width - __leadingPadding - __trailingPadding - ((__showPrefix ? 1 : 0) + (__showSuffix ? 1 : 0)) * mdState.sizeTokens.affix_spacing)
    readonly property real __prefixWidth: __showPrefix ? prefixLabel.width + mdState.sizeTokens.affix_spacing : 0
    readonly property real __suffixWidth: __showSuffix ? suffixLabel.width + mdState.sizeTokens.affix_spacing : 0
    readonly property real __inputBottomPadding: mdState.type === MD.Enum.TextFieldFilled ? mdState.verticalPadding / 2 : mdState.verticalPadding
    property MD.StateTextField mdState: MD.StateTextField {
        item: control
    }
    Binding {
        control.mdState.type: control.type
        control.mdState.error: control.error
    }

    font.capitalization: Font.MixedCase
    typescale: control.mdState.typescale
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, Math.max(contentWidth + (__showPrefix ? prefixLabel.implicitWidth + mdState.sizeTokens.affix_spacing : 0) + (__showSuffix ? suffixLabel.implicitWidth + mdState.sizeTokens.affix_spacing : 0), m_placeholder.restImplicitWidth) + __leadingPadding + __trailingPadding)
    implicitHeight: mdState.containerHeight + supportingHeight

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? m_placeholder.largestHeight / 2 : 0
    bottomInset: supportingHeight

    leftPadding: mirrored ? __trailingPadding + __suffixWidth : __leadingPadding + __prefixWidth
    rightPadding: mirrored ? __leadingPadding + __prefixWidth : __trailingPadding + __suffixWidth

    bottomPadding: __inputBottomPadding + supportingHeight
    topPadding: {
        if (mdState.type === MD.Enum.TextFieldFilled) {
            return mdState.containerHeight - contentHeight - __inputBottomPadding;
        } else {
            return mdState.verticalPadding;
        }
    }

    MD.FloatingPlaceholderText {
        id: m_placeholder
        animationsEnabled: control.__labelAnimationsEnabled
        x: control.mirrored ? control.__trailingPadding : control.__leadingPadding
        width: Math.max(0, control.width - control.__leadingPadding - control.__trailingPadding)
        horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
        text: control.placeholderText
        sourceFont: control.font
        color: control.mdState.placeholderColor
        opacity: control.mdState.placeholderOpacity
        elide: Text.ElideRight
        renderType: control.renderType

        controlFocus: control.activeFocus
        controlHeight: control.containerHeight
        verticalPadding: control.mdState.verticalPadding / 2

        filled: control.type === MD.Enum.TextFieldFilled
        controlHasText: control.length > 0
        cutoutColor: control.type === MD.Enum.TextFieldFilled ? control.mdState.backgroundColor : "transparent"
        //controlImplicitBackgroundHeight: control.implicitBackgroundHeight
    }

    property Item leading: MD.Icon {
        x: control.mirrored ? control.width - control.mdState.horizontalPadding - width : control.mdState.horizontalPadding
        anchors.verticalCenter: parent?.verticalCenter
        name: control.leadingIcon
        visible: name
        size: control.mdState.iconSize
    }

    property Item trailing: MD.Icon {
        x: control.mirrored ? control.mdState.horizontalPadding : control.width - control.mdState.horizontalPadding - width
        anchors.verticalCenter: parent?.verticalCenter
        visible: name
        name: control.trailingIcon
        size: control.mdState.iconSize
        color: control.enabled && control.error ? control.mdState.placeholderColor : MD.MProp.color.on_surface_variant
    }

    Item {
        width: control.width
        height: control.containerHeight
        data: [m_placeholder, control.leading, control.trailing]
    }

    MD.Label {
        id: prefixLabel
        text: control.prefix
        textFormat: Text.PlainText
        useTypescale: false
        font: control.font
        color: MD.MProp.color.on_surface_variant
        opacity: control.enabled ? 1 : MD.Token.state.disabled_content
        visible: control.__showPrefix
        width: Math.min(implicitWidth, control.__affixAvailableWidth)
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        x: control.mirrored ? control.width - control.__leadingPadding - width : control.__leadingPadding
        y: control.baselineOffset - baselineOffset
    }
    MD.Label {
        id: suffixLabel
        text: control.suffix
        textFormat: Text.PlainText
        useTypescale: false
        font: control.font
        color: MD.MProp.color.on_surface_variant
        opacity: control.enabled ? 1 : MD.Token.state.disabled_content
        visible: control.__showSuffix
        width: Math.min(implicitWidth, Math.max(0, control.__affixAvailableWidth - (control.__showPrefix ? prefixLabel.width : 0)))
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        x: control.mirrored ? control.__trailingPadding : control.width - control.__trailingPadding - width
        y: control.baselineOffset - baselineOffset
    }
    MD.Label {
        id: support
        x: control.mdState.horizontalPadding
        y: control.containerHeight + control.mdState.sizeTokens.supporting_top_padding
        width: Math.max(0, control.width - 2 * control.mdState.horizontalPadding)
        text: control.error && control.errorText.length > 0 ? control.errorText : control.supportingText
        textFormat: Text.PlainText
        visible: text.length > 0
        typescale: MD.Token.typescale.body_small
        color: control.mdState.supportTextColor
        opacity: control.enabled ? 1 : MD.Token.state.disabled_content
        horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
        wrapMode: Text.Wrap
    }

    cursorDelegate: MD.CursorDelegate {
        color: control.error ? MD.MProp.color.error : MD.MProp.color.primary
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
                animationsEnabled: control.__labelAnimationsEnabled
                borderColor: control.mdState.outlineColor
                radius: MD.Token.shape.corner.extra_small
                floatWidth: m_placeholder.implicitWidth + 8
                floatX: (control.mirrored ? m_placeholder.x + m_placeholder.width - m_placeholder.implicitWidth : m_placeholder.x) - 4
                open: m_placeholder.text.length > 0 && m_placeholder.floated
            }
        }
    }
    color: control.mdState.textColor
    placeholderTextColor: control.mdState.placeholderColor
}
