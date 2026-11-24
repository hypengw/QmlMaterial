pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models

import Qcm.Material as MD

/** @ingroup control */
MD.MenuBase {
    id: control

    enum Variant {
        Vertical,
        Baseline
    }

    property int variant: (parentMenu as Menu)?.variant ?? Menu.Variant.Vertical
    property bool vibrant: (parentMenu as MD.Menu)?.vibrant ?? false
    property alias mdState: item_state
    property real maximumWidth: 280
    readonly property bool cascade: true
    default property list<QtObject> entries
    property list<Item> __entryItems
    property list<Item> __ownedItems
    property list<Item> __entryOwnedItems
    property bool __entriesDirty: true
    readonly property var __itemWidths: new Map()

    function insertAction(index: int, action: MD.Action) {
        const item = delegate.createObject(contentItem, {
            action
        });
        insertItem(index, item);
        __ownedItems.push(item);
    }
    function addAction(action: MD.Action) {
        insertAction(count, action);
    }
    function insertMenu(index: int, menu: MD.MenuBase) {
        const item = delegate.createObject(contentItem, {
            text: Qt.binding(() => menu.title),
            subMenu: menu
        });
        insertItem(index, item);
        __ownedItems.push(item);
    }
    function addMenu(menu: MD.MenuBase) {
        insertMenu(count, menu);
    }
    function menuAt(index: int): MD.MenuBase {
        return (itemAt(index) as MD.MenuItemBase)?.subMenu ?? null;
    }
    function removeMenu(menu: MD.MenuBase) {
        for (let index = count - 1; index >= 0; --index) {
            if (menuAt(index) === menu)
                __removeEntry(itemAt(index));
        }
    }
    function __removeEntry(item: Item) {
        takeItem(indexOf(item));
        const entryIndex = __entryItems.indexOf(item);
        if (entryIndex >= 0)
            __entryItems.splice(entryIndex, 1);
        for (const owned of [__ownedItems, __entryOwnedItems]) {
            const index = owned.indexOf(item);
            if (index >= 0) {
                owned.splice(index, 1);
                item.destroy();
                break;
            }
        }
    }
    function actionAt(index: int): MD.Action {
        const item = itemAt(index);
        return item instanceof MD.ActionMenuItem ? item.action : null;
    }
    function removeAction(action: MD.Action) {
        for (let index = count - 1; index >= 0; --index) {
            if (actionAt(index) === action) {
                __removeEntry(itemAt(index));
            }
        }
    }
    function __syncEntries() {
        if (!__entriesDirty)
            return;
        __entriesDirty = false;
        for (const item of __entryItems)
            takeItem(indexOf(item));
        for (const item of __entryOwnedItems)
            item.destroy();
        __entryOwnedItems = [];
        __entryItems = [];
        for (const entry of entries) {
            let item = null;
            if (entry instanceof MD.Action)
                item = delegate.createObject(contentItem, {
                    action: entry
                });
            else if (entry instanceof MD.MenuBase)
                item = delegate.createObject(contentItem, {
                    text: Qt.binding(() => entry.title),
                    subMenu: entry
                });
            else if (entry instanceof Item)
                item = entry;
            if (!item)
                continue;
            if (item !== entry)
                __entryOwnedItems.push(item);
            __entryItems.push(item);
            addItem(item);
        }
    }
    onEntriesChanged: {
        __entriesDirty = true;
        Qt.callLater(__syncEntries);
    }
    onCountChanged: __syncItemWidths()
    onAboutToPrepare: {
        __syncEntries();
        __syncItemWidths();
    }

    function __syncItemWidths() {
        const items = new Set();
        for (let index = 0; index < count; ++index)
            items.add(itemAt(index));

        for (const [item, binding] of __itemWidths) {
            if (!items.has(item)) {
                binding.when = false;
                binding.destroy();
                __itemWidths.delete(item);
            }
        }
        for (const item of items) {
            if (!__itemWidths.has(item))
                __itemWidths.set(item, itemWidthBinding.createObject(control, {
                    target: item
                }));
        }
    }

    readonly property var __groups: {
        if (variant !== Menu.Variant.Vertical)
            return [];

        const groups = [];
        let first = null;
        let last = null;
        for (let index = 0; index < count; ++index) {
            const item = itemAt(index);
            if (!item?.visible)
                continue;
            if (item instanceof MD.MenuSeparator && item.gap) {
                if (first)
                    groups.push({
                        first,
                        last,
                        start: first.y,
                        end: last.y + last.height
                    });
                first = null;
            } else {
                first = first ?? item;
                last = item;
            }
        }
        if (first)
            groups.push({
                first,
                last,
                start: first.y,
                end: last.y + last.height
            });
        return groups;
    }

    readonly property bool __submenuActive: {
        if (variant !== Menu.Variant.Vertical)
            return false;
        for (let index = 0; index < count; ++index) {
            const item = itemAt(index);
            const child = item?.subMenu;
            if (child?.visible && !child.closing && child.anchorItem === item)
                return true;
        }
        return false;
    }

    property real __outerRadius: variant === Menu.Variant.Baseline || __submenuActive ? MD.MenuMetrics.itemRadius : MD.MenuMetrics.containerRadius
    Behavior on __outerRadius {
        NumberAnimation {
            duration: MD.Token.duration.short3
            easing: MD.Token.easing.standard
        }
    }

    implicitWidth: {
        const preferredWidth = Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding);
        return maximumWidth > 0 ? Math.min(preferredWidth, maximumWidth) : preferredWidth;
    }
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    margins: anchorItem ? 8 : 0
    anchorGap: 4
    verticalPadding: variant === Menu.Variant.Baseline ? MD.MenuMetrics.baselineVerticalPadding : MD.MenuMetrics.verticalInset

    transformOrigin: anchorItem ? anchorTransformOrigin : (!cascade ? Item.Top : (mirrored ? Item.TopRight : Item.TopLeft))

    delegate: MD.ActionMenuItem {}

    property var model: null
    property alias contentDelegate: m_instantiator.delegate

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            to: 0
            easing: MD.Token.easing.emphasized_accelerate
            duration: MD.Token.duration.short3
        }
        NumberAnimation {
            property: "scale"
            to: 0.8
            easing: MD.Token.easing.emphasized_accelerate
            duration: MD.Token.duration.short3
        }
    }

    contentItem: MD.ListView {
        id: menuView
        readonly property MD.Menu menu: control
        spacing: control.variant === Menu.Variant.Vertical ? MD.MenuMetrics.verticalInset : 0
        implicitWidth: {
            let preferredWidth = 0;
            for (let index = 0; index < control.count; ++index) {
                const item = control.itemAt(index);
                if (item && (!control.surfaceItem.visible || item.visible))
                    preferredWidth = Math.max(preferredWidth, item.implicitWidth);
            }
            return preferredWidth;
        }
        implicitHeight: contentHeight
        model: {
            if (!control.contentDelegate && control.model instanceof DelegateModel) {
                return control.model;
            }
            return control.contentModel;
        }
        interactive: contentHeight > control.availableHeight
        keyNavigationEnabled: false
        // Menu owns navigation; the view must not focus its first item on open.
        currentIndex: -1
        MD.ScrollIndicator.vertical: MD.ScrollIndicator {}
    }

    Instantiator {
        id: m_instantiator
        active: control.contentDelegate !== null
        model: control.model

        onObjectAdded: (index, object) => control.insertItem(index, object)
        onObjectRemoved: (index, object) => control.takeItem(control.indexOf(object))
    }

    Component {
        id: itemWidthBinding
        Binding {
            property: "width"
            value: control.availableWidth
            restoreMode: Binding.RestoreBindingOrValue
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: control.variant === Menu.Variant.Baseline ? 112 : 200
        implicitHeight: 48
        radius: control.__outerRadius
        color: control.__groups.length > 1 ? "transparent" : control.mdState.backgroundColor
        elevation: control.mdState.elevation

        Item {
            anchors.fill: parent
            visible: control.__groups.length > 1
            clip: menuView.contentHeight > menuView.height

            Repeater {
                model: control.__groups.length > 1 ? control.__groups : []
                MD.ElevationRectangle {
                    required property var modelData
                    required property int index
                    readonly property real innerRadius: Math.min(control.__outerRadius, MD.MenuMetrics.groupRadius)

                    width: parent.width
                    y: menuView.y + modelData.start - menuView.contentY - MD.MenuMetrics.verticalInset
                    height: Math.max(0, modelData.end - modelData.start) + MD.MenuMetrics.verticalInset * 2
                    corners: MD.Util.corners(index === 0 ? control.__outerRadius : innerRadius, index === 0 ? control.__outerRadius : innerRadius, index === control.__groups.length - 1 ? control.__outerRadius : innerRadius, index === control.__groups.length - 1 ? control.__outerRadius : innerRadius)
                    color: control.mdState.backgroundColor
                    elevation: control.mdState.elevation
                }
            }
        }
    }

    MD.Overlay.modal: Rectangle {
        color: MD.Util.transparent(control.MD.MProp.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short3
                easing: MD.Token.easing.linear
            }
        }
    }

    MD.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(control.MD.MProp.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short3
                easing: MD.Token.easing.linear
            }
        }
    }

    MD.MState {
        id: item_state

        target: control
        elevation: MD.Token.elevation.level2
        textColor: control.variant === Menu.Variant.Vertical && control.vibrant ? control.MD.MProp.color.on_tertiary_container : control.MD.MProp.color.on_surface
        backgroundColor: {
            const colors = control.MD.MProp.color;
            if (control.variant === Menu.Variant.Baseline)
                return colors.surface_container;
            return control.vibrant ? colors.tertiary_container : colors.surface_container_low;
        }
        supportTextColor: control.variant === Menu.Variant.Vertical && control.vibrant ? control.MD.MProp.color.on_tertiary_container : control.MD.MProp.color.on_surface_variant
    }
}
