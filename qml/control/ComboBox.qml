pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.ComboBoxBase {
    id: control
    property bool __labelAnimationsEnabled: false

    property int type: MD.Enum.TextFieldOutlined
    property real popupMaximumHeight: 0
    property real popupMaximumWidth: 280
    property string label
    property Item indicator
    property Item __indicatorItem
    readonly property real implicitIndicatorWidth: indicator ? indicator.implicitWidth : 0
    readonly property real implicitIndicatorHeight: indicator ? indicator.implicitHeight : 0
    property MD.PopupBase popup
    property MD.PopupBase __popupObject
    function __syncIndicator() {
        if (__indicatorItem === indicator)
            return;
        if (__indicatorItem && __indicatorItem.parent === control)
            __indicatorItem.parent = null;
        __indicatorItem = indicator;
        if (indicator && !indicator.parent)
            indicator.parent = control;
    }
    function __syncPopup() {
        if (__popupObject === popup)
            return;
        if (__popupObject)
            __popupObject.close();
        __popupObject = popup;
        if (popup && !popup.parent)
            popup.parent = control;
    }
    onIndicatorChanged: __syncIndicator()
    onPopupChanged: __syncPopup()
    Component.onCompleted: {
        __syncIndicator();
        __syncPopup();
        __labelAnimationsEnabled = true;
    }
    popupVisible: popup ? popup.visible : false
    popupActiveFocus: popup ? popup.activeFocus : false
    onPopupOpenRequested: if (popup) popup.open()
    onPopupCloseRequested: if (popup) popup.close()
    property MD.StateComboBox mdState: MD.StateComboBox {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: mdState.containerHeight

    font.capitalization: Font.MixedCase
    Binding {
        when: control.mdState.typescale
        control.font.pixelSize: control.mdState.typescale.size
        control.font.weight: control.mdState.typescale.weight
        control.font.letterSpacing: control.mdState.typescale.tracking
        restoreMode: Binding.RestoreNone
    }

    topInset: clip ? m_label.largestHeight / 2 : 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: mdState.horizontalPadding
    spacing: mdState.spacing
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    topPadding: 0
    bottomPadding: 0

    delegate: MD.MenuItem {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole || "modelData"]
        selected: control.currentIndex == index
        highlighted: control.highlightedIndex == index
        focusPolicy: Qt.NoFocus
    }

    indicator: MD.Icon {
        parent: control
        z: 1
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        name: MD.Token.icon.arrow_drop_down
        size: control.mdState.indicatorSize
    }

    contentItem: MD.TextInput {
        typescale: control.mdState.typescale
        clip: true

        padding: 0
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: !control.editable
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        color: control.editable ? control.mdState.textColor : "transparent"
        selectionColor: control.mdState.ctx.color.primary
        selectedTextColor: control.mdState.ctx.color.getOn(selectionColor)
        verticalAlignment: TextInput.AlignVCenter

        MD.Text {
            anchors.fill: parent
            visible: !control.editable
            text: control.displayText
            font: control.font
            color: control.mdState.textColor
            wrapMode: Text.NoWrap
            maximumLineCount: 1
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
        }
    }

    MD.FloatingPlaceholderText {
        id: m_label
        animationsEnabled: control.__labelAnimationsEnabled
        x: control.leftPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.label
        sourceFont: control.font
        color: control.mdState.labelColor
        opacity: control.mdState.labelOpacity
        elide: Text.ElideRight

        controlFocus: control.activeFocus
        controlHeight: control.height
        verticalPadding: 0

        filled: false
        controlHasText: (control.editable ? control.editText : control.displayText).length > 0
        cutoutColor: "transparent"
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: control.mdState.containerHeight
        MD.OutlineTextFieldShape {
            animationsEnabled: control.__labelAnimationsEnabled
            anchors.fill: parent
            borderColor: control.mdState.outlineColor
            radius: MD.Token.shape.corner.extra_small
            floatWidth: m_label.implicitWidth + 8
            floatX: m_label.x - 4
            open: m_label.text.length > 0 && m_label.floated
        }
    }

    popup: MD.Menu {
        id: popupMenu
        parent: control
        y: control.editable ? control.height - 5 : 0
        height: control.popupMaximumHeight > 0
            ? Math.min(implicitHeight, control.popupMaximumHeight)
            : implicitHeight
        maximumWidth: control.popupMaximumWidth
        transformOrigin: Item.Top
        modal: false
        focus: false
        model: control.delegateModel
        topMargin: 12
        bottomMargin: 12
        verticalPadding: 8
        currentIndex: control.highlightedIndex
        closePolicy: MD.PopupBase.CloseOnEscape | MD.PopupBase.CloseOnPressOutsideParent
        onOpened: contentItem.positionViewAtIndex(control.highlightedIndex, ListView.Contain)
        Connections {
            target: control
            function onHighlightedIndexChanged() {
                if (popupMenu.visible && control.highlightedIndex >= 0)
                    popupMenu.contentItem.positionViewAtIndex(control.highlightedIndex, ListView.Contain);
            }
        }
    }
}
