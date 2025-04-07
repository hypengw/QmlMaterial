import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import Qcm.Material as MD

T.ComboBox {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 12
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    topPadding: 0
    bottomPadding: 0

    delegate: MD.MenuItem {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole]
        // Material.foreground: control.currentIndex === index ? ListView.view.contentItem.Material.accent : ListView.view.contentItem.Material.foreground
        highlighted: control.highlightedIndex === index
        hoverEnabled: true//control.hoverEnabled
    }

    indicator: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        name: MD.Token.icon.arrow_drop_down
        size: 24
    }

    contentItem: MD.TextInput {
        typescale: MD.Token.typescale.body_large

        padding: 0
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: !control.editable
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        color: item_state.textColor
        selectionColor: item_state.ctx.color.primary
        selectedTextColor: item_state.ctx.color.getOn(selectionColor)
        verticalAlignment: TextInput.AlignVCenter
    }
    background: MD.MaterialTextContainer {
        implicitWidth: 64
        implicitHeight: 56

        filled: control.type === MD.Enum.TextFieldFilled
        fillColor: control.mdState.backgroundColor
        outlineColor: control.outlineColor
        focusedOutlineColor: control.outlineColor
        controlHasActiveFocus: control.activeFocus
        controlHasText: true
        horizontalPadding: 16
    }

    popup: MD.Menu {
        y: control.editable ? control.height - 5 : 0
        height: Math.min(contentItem.implicitHeight + verticalPadding * 2, control.Window.height - topMargin - bottomMargin)
        width: control.width
        transformOrigin: Item.Top
        modal: false
        model: control.delegateModel
        topMargin: 12
        bottomMargin: 12
        verticalPadding: 8
    }
    property color outlineColor: item_state.outlineColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level0
        textColor: item_state.ctx.color.on_surface
        backgroundColor: "transparent"
        supportTextColor: item_state.ctx.color.on_surface_variant
        outlineColor: item_state.ctx.color.outline

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.supportTextColor: item_state.ctx.color.on_surface
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Error"
                when: !control.acceptableInput && !control.hovered
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: item_state.ctx.color.error
                    item_state.outlineColor: item_state.ctx.color.error
                }
            },
            State {
                name: "ErrorHover"
                when: !control.acceptableInput && control.hovered
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: item_state.ctx.color.error
                    item_state.outlineColor: item_state.ctx.color.on_error_container
                }
            },
            State {
                name: "Focused"
                when: control.visualFocus
                PropertyChanges {
                    item_state.outlineColor: item_state.ctx.color.primary
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.outlineColor: item_state.ctx.color.on_surface
                }
            }
        ]
    }
}
